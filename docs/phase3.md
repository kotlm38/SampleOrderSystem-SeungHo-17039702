# Phase 3 — Controller 레이어

## 목표
비즈니스 로직을 담당하는 5개 컨트롤러를 구현한다.  
컨트롤러는 DataStore를 통해서만 데이터에 접근하며, View에 의존하지 않는다.

---

## 파일 목록

| 파일 | 역할 |
|------|------|
| `SampleController.h / .cpp` | 시료 등록·조회·검색 |
| `OrderController.h / .cpp` | 주문 접수·승인·거절 |
| `ProductionController.h / .cpp` | 백그라운드 생산 스레드 |
| `MonitoringController.h / .cpp` | 상태별 주문 조회, 재고 현황 |
| `ShippingController.h / .cpp` | 출고 처리 |

---

## 3-1. SampleController

### 헤더

```cpp
#pragma once
#include "../Model/Sample.h"
#include <vector>
#include <string>

enum class SampleSearchField {
    Id,
    Name,
    AvgProductionTimeSec,
    Yield
};

class SampleController {
public:
    bool                registerSample(const std::string& id,
                                       const std::string& name,
                                       int avgProductionTimeSec,
                                       float yield);
    std::vector<Sample> getAllSamples() const;
    std::vector<Sample> search(SampleSearchField field,
                               const std::string& keyword) const;
};
```

### 구현 요점

- `registerSample`: `findSample(id)` 존재 여부 확인 → 중복이면 false 반환
- 초기 stock은 0

#### search()
```
전체 목록에서 field에 따라 keyword를 포함하는 항목만 반환.

문자열 속성 (Id, Name):
    attribute.find(keyword) != npos

숫자 속성 (AvgProductionTimeSec, Yield):
    std::to_string(attribute).find(keyword) != npos
    // 부분 문자열 일치 — 예: keyword "1" 은 avgTime 10, 15 모두 매칭
```

---

## 3-2. OrderController

### 헤더

```cpp
#pragma once
#include "../Model/Order.h"
#include <vector>
#include <string>

class OrderController {
public:
    // 고객 주문 접수 → Reserved
    std::string        createOrder(const std::string& customer,
                                   const std::string& sampleId,
                                   int quantity);

    // 주문 승인: 재고 충분 → Confirmed / 부족 → Producing
    bool               approveOrder(const std::string& orderId);

    // 주문 거절 → DataStore에서 삭제
    bool               rejectOrder(const std::string& orderId);

    // Reserved 상태 주문 목록
    std::vector<Order> getReservedOrders() const;

private:
    std::string        generateOrderId() const;
};
```

**POC 대비 변경:** `createOrder`에 `customer` 파라미터 추가.

### 구현 요점

#### generateOrderId()
```cpp
// "ORD-{timestamp}-{counter}" 형식
// 같은 초에 여러 주문 생성 시 counter로 구분
static std::atomic<int> s_counter{ 0 };
return "ORD-" + std::to_string(std::time(nullptr))
             + "-" + std::to_string(s_counter++);
```

#### createOrder()
1. `findSample(sampleId)` → 존재하지 않으면 빈 문자열 반환
2. Order 구성 (status = Reserved, createdAt = updatedAt = now)
3. `DataStore::addOrder(order)`
4. orderId 반환

#### approveOrder()
1. `findOrder(orderId)` / `findSample(sampleId)` 확인
2. `sample.stock >= order.quantity` 판단
   - **재고 충분:** stock 차감 → order.status = Confirmed → updateSample + updateOrder
   - **재고 부족:** order.status = Producing → updateOrder  
     → 부족분 = order.quantity - sample.stock  
     → 생산량 = ceil(부족분 / (sample.yield × 0.9))  
     → **sample.stock = 0 → updateSample(sample)**  // 남은 재고 전량 선점: 다른 주문이 동일 재고를 중복 사용하지 못하도록 즉시 차감  
     → ProductionJob 생성 (jobId = "JOB-" + orderId, quantity = 생산량, **shortfall = 부족분**, **startTime = 0**, durationSec = avgProductionTimeSec × 생산량)  
     → `DataStore::addProductionJob(job)`  
     // shortfall: onJobComplete에서 재고 차감 시 사용 (phase2 ProductionJob 구조체에 shortfall 필드 추가 필요)  
     // startTime = 0: 대기 상태 마커 — workerLoop이 최초 처리 시 실제 시각으로 갱신

#### rejectOrder()
- `findOrder` 확인 후 `DataStore::removeOrder(orderId)`
- 주문을 DataStore에서 영구 삭제한다. **시스템에서 주문이 삭제되는 유일한 시점.**

---

## 3-3. ProductionController

### 헤더

```cpp
#pragma once
#include "../Model/ProductionJob.h"
#include <vector>
#include <thread>
#include <atomic>

class ProductionController {
public:
    static ProductionController& instance();

    void start();
    void stop();

    std::vector<ProductionJob> getActiveJobs()  const;  // startTime > 0: 생산 진행 중
    std::vector<ProductionJob> getPendingJobs() const;  // startTime == 0: 생산 대기 중

private:
    ProductionController() = default;
    ProductionController(const ProductionController&) = delete;
    ProductionController& operator=(const ProductionController&) = delete;

    void workerLoop();
    void onJobComplete(const ProductionJob& job);

    std::thread        worker_;
    std::atomic<bool>  running_{ false };
};
```

### 구현 요점

#### workerLoop()

PRD 요구사항: "생산은 한 종류의 시료씩 이루어지며 주문 입력 순서대로(FIFO방식으로) 처리한다."
- 활성 Job(startTime > 0)이 존재하는 동안은 대기 Job을 새로 시작하지 않는다.
- 대기 Job 활성화 시 목록 첫 번째 항목만 선택(FIFO).
- 완료 처리를 먼저 수행한 뒤 새 Job 시작 여부를 결정한다.

```
while (running_):
    sleep 1초
    now = time(nullptr)

    // 1단계: 완료된 Job 처리
    jobs = DataStore::getProductionJobs()
    for job in jobs:
        if job.startTime > 0 and now >= job.startTime + job.durationSec:
            onJobComplete(job)

    // 2단계: FIFO — 활성 Job이 없을 때만 대기 중 첫 번째 Job 시작
    // phase2 DataStore에 updateProductionJob() 추가 필요
    jobs = DataStore::getProductionJobs()   // 완료 처리 후 재조회
    hasActive = any job where job.startTime > 0
    if not hasActive:
        firstPending = first job where job.startTime == 0  // 삽입 순서 = FIFO
        if firstPending exists:
            firstPending.startTime = now
            DataStore::updateProductionJob(firstPending)
```

#### getActiveJobs() / getPendingJobs()
```
getActiveJobs():  전체 Job 중 startTime > 0 인 항목만 반환
getPendingJobs(): 전체 Job 중 startTime == 0 인 항목만 반환
```

#### onJobComplete()
```
1. findSample(job.sampleId)
2. 수율 적용: producedQty = ceil(job.quantity * sample.yield)
   // floor 대신 ceil을 사용해 부동소수점 오차로 인한 미달을 원천 차단한다.
   // approveOrder의 생산량 수식: job.quantity = ceil(shortfall / (yield * 0.9))
   // 이므로 ceil 적용 후에도 producedQty >= shortfall 이 수학적으로 보장된다.
   sample.stock += producedQty
   updateSample(sample)

3. findOrder(job.orderId)
4. 부족분만 차감 후 Confirmed 전환
   // approveOrder()에서 기존 재고를 stock=0으로 선점했으므로
   // 여기서는 실제 부족분(job.shortfall)만 차감한다.
   // 잉여 생산분(producedQty - job.shortfall)은 재고로 남는다.
   sample.stock -= job.shortfall
   updateSample(sample)
   order.status = Confirmed
   order.updatedAt = now
   updateOrder(order)

5. removeProductionJob(job.jobId)
```

**주의:** onJobComplete 내에서 DataStore를 여러 번 호출하는 사이에 다른 스레드가 개입할 수 있다.  
각 DataStore 메서드가 개별 락을 가지므로, 두 호출 사이 짧은 inconsistency는 허용 범위로 간주한다.  
(단일 원자 트랜잭션이 필요하다면 DataStore에 배치 API를 추가하는 방향으로 확장 가능)

---

## 3-4. MonitoringController

### 헤더

```cpp
#pragma once
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include <vector>

enum class StockStatus {
    Sufficient,  // 재고 >= 활성 주문 총 요구량
    Shortage,    // 0 < 재고 < 요구량
    Depleted     // 재고 == 0
};

struct InventoryInfo {
    Sample      sample;
    StockStatus stockStatus;
};

class MonitoringController {
public:
    std::vector<Order>         getOrdersByStatus(OrderStatus status) const;
    std::vector<InventoryInfo> getInventoryStatus() const;
};
```

### 구현 요점

#### getInventoryStatus()
- 활성 주문(Reserved + Producing) 총 요구량 집계
- `stock == 0` → Depleted
- `0 < stock < demanded` → Shortage
- `stock >= demanded` → Sufficient

POC와 동일한 로직.

---

## 3-5. ShippingController

### 헤더

```cpp
#pragma once
#include "../Model/Order.h"
#include <vector>
#include <string>

class ShippingController {
public:
    std::vector<Order> getConfirmedOrders() const;
    bool               processShipping(const std::string& orderId);
};
```

### 구현 요점

#### processShipping()
1. `findOrder(orderId)` → 없으면 false
2. `order.status != Confirmed` → false
3. `order.status = Release`, `order.updatedAt = now`
4. `DataStore::updateOrder(order)` — 상태 변경만 수행, **`removeOrder`를 호출하지 않는다**
5. true 반환

Release 상태 주문은 출고 이력 보존을 위해 DataStore에 유지된다.  
POC와 동일한 로직.

---

## 완료 기준

- `SampleController::registerSample`: 중복 ID 거부, 정상 등록 확인
- `OrderController::approveOrder`: 재고 있을 때 Confirmed, 없을 때 Producing + Job 생성 확인
- `OrderController::approveOrder` (재고 부족 분기): 승인 직후 sample.stock == 0 확인 (선점)
- `ProductionController`: 생성 직후 Job이 getPendingJobs()에 포함되고, workerLoop 처리 후 getActiveJobs()로 이동 확인
- `ProductionController`: 백그라운드에서 durationSec 경과 후 자동 Confirmed 전환 확인
- `ProductionController` (FIFO): 대기 Job이 복수일 때 동시에 하나만 활성화되는지 확인; 첫 번째 Job 완료 후 두 번째 Job이 활성화되는지 확인
- `ProductionController::onJobComplete`: producedQty(ceil 적용)가 항상 shortfall 이상임을 확인; 잉여분은 재고로 누적되는지 확인
- `MonitoringController`: 재고 상태 여유/부족/고갈 분류 정확성 확인
- `ShippingController`: Confirmed → Release 전환, 비-Confirmed 주문은 거부 확인
