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

class SampleController {
public:
    bool                registerSample(const std::string& id,
                                       const std::string& name,
                                       int avgProductionTimeSec,
                                       float yield);
    std::vector<Sample> getAllSamples() const;
    std::vector<Sample> searchByName(const std::string& keyword) const;
};
```

### 구현 요점

- `registerSample`: `findSample(id)` 존재 여부 확인 → 중복이면 false 반환
- `searchByName`: 전체 목록에서 `name.find(keyword) != npos` 필터링
- 초기 stock은 0

POC와 동일한 로직.

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
     → ProductionJob 생성 (jobId = "JOB-" + orderId, durationSec = avgProductionTimeSec × quantity)  
     → `DataStore::addProductionJob(job)`

#### rejectOrder()
- `findOrder` 확인 후 `DataStore::removeOrder(orderId)`

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

    std::vector<ProductionJob> getActiveJobs() const;

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
```
while (running_):
    sleep 1초
    jobs = DataStore::getProductionJobs()
    now  = time(nullptr)
    for job in jobs:
        if now >= job.startTime + job.durationSec:
            onJobComplete(job)
```

#### onJobComplete()
```
1. findSample(job.sampleId)
2. 수율 적용: producedQty = floor(job.quantity * sample.yield)
   sample.stock += producedQty
   updateSample(sample)

3. findOrder(job.orderId)
4. 생산된 수량에서 주문 수량 차감
   if sample.stock >= order.quantity:
       sample.stock -= order.quantity
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
4. `DataStore::updateOrder(order)`
5. true 반환

POC와 동일한 로직.

---

## 완료 기준

- `SampleController::registerSample`: 중복 ID 거부, 정상 등록 확인
- `OrderController::approveOrder`: 재고 있을 때 Confirmed, 없을 때 Producing + Job 생성 확인
- `ProductionController`: 백그라운드에서 durationSec 경과 후 자동 Confirmed 전환 확인
- `MonitoringController`: 재고 상태 여유/부족/고갈 분류 정확성 확인
- `ShippingController`: Confirmed → Release 전환, 비-Confirmed 주문은 거부 확인
