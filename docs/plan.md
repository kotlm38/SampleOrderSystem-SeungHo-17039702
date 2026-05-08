# Semi 프로젝트 구현 계획

## 개요

반도체 시료 생산/출고 콘솔 프로그램을 C++20으로 구현한다.  
`POC_MVC`의 아키텍처를 기반으로 `Semi` 프로젝트에 완전한 코드를 작성한다.  
POC와의 핵심 차이는 **JSON 영속화 완전 구현**과 **PRD 기반 누락 필드 보완**이다.

---

## 아키텍처 (MVC)

```
main.cpp
├── Model/          데이터 구조체 + DataStore (JSON 영속화)
├── Controller/     비즈니스 로직
└── View/           콘솔 I/O
```

백그라운드 스레드(`ProductionController`)가 생산 큐를 1초 간격으로 폴링하며,  
메인 스레드는 콘솔 메뉴 루프를 실행한다. 두 스레드는 DataStore의 mutex로 동기화한다.

---

## 파일 구조

```
Semi/
├── main.cpp
├── Lib/
│   └── json.hpp                  (nlohmann/json single-header)
├── Model/
│   ├── Sample.h
│   ├── Order.h / Order.cpp
│   ├── ProductionJob.h
│   └── DataStore.h / DataStore.cpp
├── Controller/
│   ├── SampleController.h / .cpp
│   ├── OrderController.h / .cpp
│   ├── ProductionController.h / .cpp
│   ├── MonitoringController.h / .cpp
│   └── ShippingController.h / .cpp
└── View/
    ├── MainMenuView.h / .cpp
    ├── SampleView.h / .cpp
    ├── OrderView.h / .cpp
    ├── MonitoringView.h / .cpp
    ├── ShippingView.h / .cpp
    └── ProductionLineView.h / .cpp
```

데이터 파일: `DB/data.json` (솔루션 루트 기준)

---

## POC_MVC 대비 변경/보완 사항

| 항목 | POC_MVC | Semi |
|------|---------|------|
| DataStore load/save | TODO 스텁 | nlohmann/json으로 완전 구현 |
| Order.customer | 없음 | 추가 (PRD: "주문 : 번호, 고객, 시료, 수량, 상태") |
| 주문 ID 중복 방지 | 타임스탬프만 사용 | 타임스탬프 + 카운터로 동일 초 충돌 방지 |
| DB 경로 | 하드코딩 "data.json" | 실행파일 위치 기준 `../DB/data.json` |

---

## 데이터 구조

### Sample
```cpp
struct Sample {
    std::string id;
    std::string name;
    int         avgProductionTimeSec;
    float       yield;        // 0.0 ~ 1.0
    int         stock;
};
```

### Order
```cpp
struct Order {
    std::string orderId;
    std::string customer;     // PRD 명시 필드 (POC에서 누락)
    std::string sampleId;
    int         quantity;
    OrderStatus status;       // Reserved / Rejected / Producing / Confirmed / Release
    std::time_t createdAt;
    std::time_t updatedAt;
};
```

### ProductionJob
```cpp
struct ProductionJob {
    std::string jobId;
    std::string orderId;
    std::string sampleId;
    int         quantity;
    std::time_t startTime;
    int         durationSec;  // avgProductionTimeSec * quantity
};
```

### DB/data.json 스키마
```json
{
  "samples": [
    { "id": "S001", "name": "GaN Wafer", "avgProductionTimeSec": 10,
      "yield": 0.9, "stock": 50 }
  ],
  "orders": [
    { "orderId": "ORD-1000-0", "customer": "삼성전자", "sampleId": "S001",
      "quantity": 5, "status": "Reserved",
      "createdAt": 1715000000, "updatedAt": 1715000000 }
  ],
  "productionJobs": [
    { "jobId": "JOB-ORD-1000-0", "orderId": "ORD-1000-0",
      "sampleId": "S001", "quantity": 5,
      "startTime": 1715000010, "durationSec": 50 }
  ]
}
```

---

## 구현 단계

### Phase 1 — 환경 셋업
1. `Semi/Lib/json.hpp` 추가 (nlohmann/json v3.x single-header 다운로드)
2. `Semi.vcxproj`에 모든 .cpp/.h 파일 추가
3. 추가 include 경로 설정 (`Semi/Lib`)
4. `DB/` 폴더 생성 확인 (없으면 DataStore::save에서 생성)

### Phase 2 — Model 레이어
1. `Sample.h` — POC와 동일
2. `Order.h / Order.cpp` — customer 필드 추가, status 직렬화 함수 포함
3. `ProductionJob.h` — POC와 동일
4. `DataStore.h` — POC와 동일
5. `DataStore.cpp` — `load()` / `save()` JSON 구현
   - 파일 없을 경우 빈 상태로 시작
   - save는 atomic write (임시파일 → rename) 고려

### Phase 3 — Controller 레이어
1. `SampleController` — POC와 동일
2. `OrderController` — createOrder에 customer 파라미터 추가
3. `ProductionController` — POC와 동일 (백그라운드 스레드)
4. `MonitoringController` — POC와 동일
5. `ShippingController` — POC와 동일

### Phase 4 — View 레이어
1. `MainMenuView` — POC와 동일
2. `SampleView` — POC와 동일
3. `OrderView` — doCreateOrder에 고객명 입력 추가
4. `MonitoringView` — POC와 동일
5. `ShippingView` — POC와 동일
6. `ProductionLineView` — POC와 동일

### Phase 5 — 진입점
- `main.cpp` — POC와 동일

---

## 주요 설계 결정

### JSON 라이브러리
- **nlohmann/json** (header-only, MIT)
- 빌드 시스템 변경 없이 `json.hpp` 한 파일만 추가

### 스레드 안전성
- DataStore의 모든 public 메서드는 `std::mutex` + `lock_guard`로 보호
- `save()`는 락을 잡은 내부 helper(`saveUnlocked`)와 락을 잡는 외부 wrapper로 분리  
  (addSample 등에서 이미 락 보유 상태로 save가 호출되면 deadlock 발생 방지)

### 주문 ID 생성
```
"ORD-{timestamp}-{counter}"
```
같은 초에 여러 주문이 생성되어도 counter로 구분

### 생산 완료 조건
```cpp
std::time(nullptr) >= job.startTime + job.durationSec
```
workerLoop에서 1초 간격 폴링. 완료된 job은 수율 적용 후 재고 증가 → 주문 Confirmed

### DB 경로
- DataStore에서 실행파일 기준 상대경로 `../DB/data.json` 사용
- Windows: `_pgmptr` 또는 `GetModuleFileName`으로 실행파일 경로 취득

---

## 메뉴 구조

```
메인 메뉴
├── 1. 시료 관리
│   ├── 1. 시료 등록 (ID, 이름, 평균생산시간, 수율)
│   ├── 2. 시료 조회 (전체 목록 + 재고)
│   └── 3. 시료 검색 (이름 부분 검색)
├── 2. 주문 (접수/승인/거절)
│   ├── 1. 주문 접수 (고객명, 시료ID, 수량 → Reserved)
│   └── 2. 주문 승인/거절 (Reserved 목록 표시 → 승인/거절 선택)
│       └── 승인 시: 재고 있으면 Confirmed, 없으면 Producing + 생산큐 등록
├── 3. 모니터링
│   ├── 1. 주문량 확인 (상태별 목록)
│   └── 2. 재고량 확인 (시료별 재고 + 상태: 여유/부족/고갈)
├── 4. 출고 처리 (Confirmed → Release)
├── 5. 생산 라인 (현재 생산 중인 Job 목록 + 남은 시간)
└── 0. 종료
```

---

## 구현 순서 요약

| 순서 | 작업 | 비고 |
|------|------|------|
| 1 | Lib/json.hpp 추가 | nlohmann/json 다운로드 |
| 2 | Model 헤더 3종 | Sample, Order, ProductionJob |
| 3 | DataStore | load/save JSON 구현 포함 |
| 4 | Controller 5종 | POC 기반, customer 파라미터 보완 |
| 5 | View 6종 | POC 기반, 고객명 입력 보완 |
| 6 | main.cpp | POC와 동일 |
| 7 | Semi.vcxproj | 파일 목록 추가 |
