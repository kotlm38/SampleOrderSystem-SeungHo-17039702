# Phase 2 — Model 레이어

## 목표
데이터 구조체와 JSON 영속화를 담당하는 DataStore를 완전히 구현한다.  
모든 상태 변경은 DataStore를 통하며, 변경 즉시 `DB/data.json`에 반영된다.

---

## 파일 목록

| 파일 | 설명 |
|------|------|
| `Semi/Model/Sample.h` | 시료 구조체 |
| `Semi/Model/Order.h` | 주문 구조체 + OrderStatus 열거형 |
| `Semi/Model/Order.cpp` | OrderStatus 직렬화 함수 |
| `Semi/Model/ProductionJob.h` | 생산 작업 구조체 |
| `Semi/Model/DataStore.h` | 싱글톤 DataStore 선언 |
| `Semi/Model/DataStore.cpp` | DataStore 구현 (JSON load/save) |

---

## 2-1. Sample.h

```cpp
#pragma once
#include <string>

struct Sample {
    std::string id;
    std::string name;
    int         avgProductionTimeSec;
    float       yield;   // 0.0 ~ 1.0
    int         stock;
};
```

POC와 동일. 변경 없음.

---

## 2-2. Order.h

```cpp
#pragma once
#include <string>
#include <ctime>

enum class OrderStatus {
    Reserved,
    Rejected,
    Producing,
    Confirmed,
    Release
};

std::string orderStatusToString(OrderStatus status);
OrderStatus orderStatusFromString(const std::string& str);

struct Order {
    std::string orderId;
    std::string customer;   // PRD 명시 필드 — POC에서 누락
    std::string sampleId;
    int         quantity;
    OrderStatus status;
    std::time_t createdAt;
    std::time_t updatedAt;
};
```

**POC 대비 변경:** `customer` 필드 추가.

---

## 2-3. Order.cpp

```cpp
#include "Order.h"

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Reserved:  return "Reserved";
        case OrderStatus::Rejected:  return "Rejected";
        case OrderStatus::Producing: return "Producing";
        case OrderStatus::Confirmed: return "Confirmed";
        case OrderStatus::Release:   return "Release";
        default:                     return "Unknown";
    }
}

OrderStatus orderStatusFromString(const std::string& str) {
    if (str == "Reserved")  return OrderStatus::Reserved;
    if (str == "Rejected")  return OrderStatus::Rejected;
    if (str == "Producing") return OrderStatus::Producing;
    if (str == "Confirmed") return OrderStatus::Confirmed;
    if (str == "Release")   return OrderStatus::Release;
    return OrderStatus::Reserved;
}
```

---

## 2-4. ProductionJob.h

```cpp
#pragma once
#include <string>
#include <ctime>

struct ProductionJob {
    std::string  jobId;
    std::string  orderId;
    std::string  sampleId;
    int          quantity;     // 실제 생산할 수량 = ceil(shortfall / (yield * 0.9))
    int          shortfall;    // 주문 부족분 = order.quantity - 승인 시 sample.stock
                               // onJobComplete에서 재고 차감 기준으로 사용
    std::time_t  startTime;    // 0 = 대기 중(Pending), >0 = 생산 시작 시각
    int          durationSec;  // avgProductionTimeSec * quantity
};
```

**POC 대비 변경:**
- `shortfall` 추가 — 재고 선점(stock=0) 후 onJobComplete에서 실제 차감량 참조
- `startTime = 0` 을 "대기 중" 마커로 사용 — workerLoop이 처리 시 실제 시각으로 갱신

---

## 2-5. DataStore.h

```cpp
#pragma once
#include "Sample.h"
#include "Order.h"
#include "ProductionJob.h"
#include <vector>
#include <mutex>
#include <string>
#include <optional>

class DataStore {
public:
    static DataStore& instance();

    void load();
    void save();

    // --- Sample ---
    std::vector<Sample>        getSamples() const;
    std::optional<Sample>      findSample(const std::string& id) const;
    void                       addSample(const Sample& s);
    void                       updateSample(const Sample& s);

    // --- Order ---
    std::vector<Order>         getOrders() const;
    std::optional<Order>       findOrder(const std::string& orderId) const;
    void                       addOrder(const Order& o);
    void                       updateOrder(const Order& o);
    void                       removeOrder(const std::string& orderId);

    // --- ProductionJob ---
    std::vector<ProductionJob> getProductionJobs() const;
    void                       addProductionJob(const ProductionJob& job);
    void                       updateProductionJob(const ProductionJob& job);  // startTime 갱신(대기→생산 전환)에 사용
    void                       removeProductionJob(const std::string& jobId);

private:
    DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    void saveUnlocked() const;          // 락 없이 파일에 쓰기 (내부 전용)
    static std::string dbFilePath();    // DB/data.json 경로 계산

    mutable std::mutex          mutex_;
    std::vector<Sample>         samples_;
    std::vector<Order>          orders_;
    std::vector<ProductionJob>  jobs_;
};
```

**POC 대비 변경:**
- `saveUnlocked()` 추가 — add*/update*/remove* 내부에서 이미 락을 잡은 채로 저장할 때 deadlock 방지
- `dbFilePath()` 추가 — 실행 경로 기준으로 DB 폴더를 찾음

---

## 2-6. DataStore.cpp

### 경로 계산 (`dbFilePath`)

```cpp
// Windows: 실행파일 위치에서 ../DB/data.json
static std::string DataStore::dbFilePath() {
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string exe(buf);
    // 마지막 '\' 이후 제거 → 실행파일 디렉토리
    auto pos = exe.rfind('\\');
    std::string dir = (pos != std::string::npos) ? exe.substr(0, pos) : ".";
    return dir + "\\..\\DB\\data.json";
}
```

디버그 빌드 실행 경로(`Semi/x64/Debug/`)에서 `../DB/data.json`은  
솔루션 루트의 `DB/data.json`을 가리킨다.

### DB 폴더 자동 생성

```cpp
// saveUnlocked 상단에서 호출
// <filesystem> 또는 Windows API CreateDirectoryA 사용
static void ensureDbDir(const std::string& filePath) {
    // filePath에서 디렉터리 부분 추출 후 CreateDirectoryA
}
```

`<filesystem>`(C++17)을 사용하면 `std::filesystem::create_directories()`로 간단히 처리 가능.

### load() 구현

```cpp
void DataStore::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream ifs(dbFilePath());
    if (!ifs.is_open()) return;   // 파일 없으면 빈 상태 유지

    json j = json::parse(ifs, nullptr, false);  // 파싱 실패 시 예외 대신 discarded 반환
    if (j.is_discarded()) return;

    // samples
    for (auto& s : j.value("samples", json::array())) {
        samples_.push_back({
            s["id"], s["name"],
            s["avgProductionTimeSec"].get<int>(),
            s["yield"].get<float>(),
            s["stock"].get<int>()
        });
    }
    // orders
    for (auto& o : j.value("orders", json::array())) {
        Order ord;
        ord.orderId   = o["orderId"];
        ord.customer  = o["customer"];
        ord.sampleId  = o["sampleId"];
        ord.quantity  = o["quantity"].get<int>();
        ord.status    = orderStatusFromString(o["status"].get<std::string>());
        ord.createdAt = o["createdAt"].get<std::time_t>();
        ord.updatedAt = o["updatedAt"].get<std::time_t>();
        orders_.push_back(ord);
    }
    // productionJobs
    for (auto& job : j.value("productionJobs", json::array())) {
        ProductionJob pj;
        pj.jobId       = job["jobId"];
        pj.orderId     = job["orderId"];
        pj.sampleId    = job["sampleId"];
        pj.quantity    = job["quantity"].get<int>();
        pj.shortfall   = job["shortfall"].get<int>();
        pj.startTime   = job["startTime"].get<std::time_t>();
        pj.durationSec = job["durationSec"].get<int>();
        jobs_.push_back(pj);
    }
}
```

### saveUnlocked() 구현

쓰기 도중 프로세스가 종료되어도 기존 파일이 손상되지 않도록 **임시파일 → rename** 방식(atomic write)을 사용한다.

```cpp
void DataStore::saveUnlocked() const {
    json j;

    json samplesArr = json::array();
    for (const auto& s : samples_) {
        samplesArr.push_back({
            {"id",                    s.id},
            {"name",                  s.name},
            {"avgProductionTimeSec",  s.avgProductionTimeSec},
            {"yield",                 s.yield},
            {"stock",                 s.stock}
        });
    }
    j["samples"] = samplesArr;

    json ordersArr = json::array();
    for (const auto& o : orders_) {
        ordersArr.push_back({
            {"orderId",   o.orderId},
            {"customer",  o.customer},
            {"sampleId",  o.sampleId},
            {"quantity",  o.quantity},
            {"status",    orderStatusToString(o.status)},
            {"createdAt", o.createdAt},
            {"updatedAt", o.updatedAt}
        });
    }
    j["orders"] = ordersArr;

    json jobsArr = json::array();
    for (const auto& pj : jobs_) {
        jobsArr.push_back({
            {"jobId",       pj.jobId},
            {"orderId",     pj.orderId},
            {"sampleId",    pj.sampleId},
            {"quantity",    pj.quantity},
            {"shortfall",   pj.shortfall},
            {"startTime",   pj.startTime},
            {"durationSec", pj.durationSec}
        });
    }
    j["productionJobs"] = jobsArr;

    std::string target = dbFilePath();
    std::string tmp    = target + ".tmp";

    ensureDbDir(target);

    // 1단계: 임시 파일에 쓰기
    {
        std::ofstream ofs(tmp);
        ofs << j.dump(2);   // 들여쓰기 2칸
    }   // ofs 소멸 시 flush + close 보장

    // 2단계: 임시 파일을 대상 파일로 원자적 교체
    // MoveFileExA는 같은 볼륨 내에서 atomic rename을 보장한다.
    MoveFileExA(tmp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING);
}
```

### save() — 외부 공개 버전

```cpp
void DataStore::save() {
    std::lock_guard<std::mutex> lock(mutex_);
    saveUnlocked();
}
```

### updateProductionJob 구현

```cpp
void DataStore::updateProductionJob(const ProductionJob& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pj : jobs_) {
        if (pj.jobId == job.jobId) { pj = job; break; }
    }
    saveUnlocked();
}
```

`updateOrder` / `updateSample`과 동일한 패턴. jobId로 기존 항목을 찾아 교체 후 즉시 저장.

### addSample 패턴 (deadlock 방지)

```cpp
void DataStore::addSample(const Sample& s) {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.push_back(s);
    saveUnlocked();   // ← lock 내부에서 saveUnlocked 호출 (재귀 lock 없음)
}
```

모든 add*/update*/remove* 메서드가 이 패턴을 따른다.

---

## 완료 기준

- `DataStore::load()` 호출 후 `getSamples()` 등이 JSON에서 복원한 데이터를 반환
- `addSample()` 등 호출 직후 `DB/data.json`이 갱신됨
- 프로그램 재시작 후에도 이전 상태가 복원됨
- `DB/data.json` 미존재 시 load()가 오류 없이 빈 상태를 유지
