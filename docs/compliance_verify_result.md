# Semi 프로젝트 Compliance Verify 결과 보고서

검사 일시: 2026-05-08  
검사 대상: `C:\reviewer\Semi\Semi\` 구현 코드 ↔ `CLAUDE.md`, `SemiPRD.md`, `docs/plan.md`, `docs/phase1~5.md`

---

## 요약

| 항목 | 건수 |
|------|------|
| CRITICAL | 0 |
| MAJOR | 3 |
| MINOR | 4 |
| AMBIGUOUS | 4 |
| INFO | 3 |
| 통과 | 29 |

전체 구현 완료율: **100%** (기대 파일 26개 / 실제 파일 26개 모두 존재)

---

## 상세 결과

### [MAJOR-1] DB/data.json에 `customer` 필드 누락

- **문서 근거:** `SemiPRD.md` "주문: 번호, 고객, 시료, 수량, 상태", `docs/plan.md` Order 구조체 `customer` 필드 명시, `docs/phase2.md` saveUnlocked() JSON 직렬화 코드에 `{"customer", o.customer}` 명시
- **코드 실제 동작:** `Semi/Model/DataStore.cpp:97~106` — 코드 자체는 `customer` 필드를 올바르게 직렬화함
- **차이:** `DB/data.json`의 기존 레코드 전체에 `customer` 필드가 없음. 코드의 `DataStore::load()`에서 `ord.customer = o["customer"].get<std::string>()`를 직접 키 접근으로 파싱하므로, 이 파일을 그대로 로드하면 `nlohmann::json` 라이브러리가 예외를 던지거나 비정상 동작할 수 있음
- **영향:** 현재 저장된 `DB/data.json`을 로드하면 `customer` 키 없음으로 인해 런타임 예외 또는 잘못된 데이터 복원이 발생할 수 있음. 기존 데이터가 사실상 로드 불가 상태임

### [MAJOR-2] DB/data.json에 `productionJobs`의 `shortfall` 필드 누락

- **문서 근거:** `docs/plan.md` DB/data.json 스키마에 `"shortfall": 5` 명시, `docs/phase2.md` saveUnlocked() 코드에 `{"shortfall", pj.shortfall}` 포함
- **코드 실제 동작:** `Semi/Model/DataStore.cpp:109~121` — 코드 자체는 `shortfall`을 직렬화함
- **차이:** `DB/data.json`의 `productionJobs` 배열에 `shortfall` 필드가 없음. 코드 로드 시 `pj.shortfall = job["shortfall"].get<int>()` 라인이 키 없음으로 예외 발생 가능
- **영향:** 기존 저장된 생산 작업 데이터를 로드할 경우 런타임 예외. `shortfall` 값이 0으로 대체되면 `onJobComplete`에서 재고를 전혀 차감하지 않아 재고 과다 적립 오류 발생

### [MAJOR-3] `ShippingView::show()`에서 `getConfirmedOrders()` 이중 호출로 인한 경쟁 조건

- **문서 기대 동작:** `docs/phase4.md` ShippingView — "printConfirmedOrders(): Confirmed 목록 출력 → 목록 없으면 반환 → 주문번호 입력 → processShipping() 호출"
- **코드 실제 동작:** `Semi/View/ShippingView.cpp:6~21` — `printConfirmedOrders()`와 `show()` 내부에서 `getConfirmedOrders()`를 각각 호출함
- **차이:** 두 번의 호출 사이에 백그라운드 스레드가 상태를 변경할 경우 목록은 보이나 입력 기회를 잃는 UI 불일치 가능성 존재. 불필요한 DataStore 락 획득이 2회 발생하여 효율 저하

---

### [MINOR-1] 모니터링 주문량 확인에서 Rejected 상태 미표시

- **문서:** `SemiPRD.md` "모든 주문은 5가지 Status(Reserved, Rejected, Producing, Confirmed, Release) 중 하나를 보유한다", "Rejected: 주문이 거절된 상태. 더이상 관리하지 않고 주문 목록에서 삭제."
- **코드:** `Semi/View/MonitoringView.cpp:41~43` — statuses 배열에 `Reserved`, `Producing`, `Confirmed`, `Release` 4가지만 포함
- **차이:** Rejected는 주문 즉시 삭제되므로 모니터링에 표시될 데이터가 없다는 것은 PRD 정의에 부합하나, 표시되는 상태 목록이 PRD 5가지에서 누락된 것처럼 보일 수 있어 명확성 부족

### [MINOR-2] `ProductionLineView`에서 시료 조회 시 `getAllSamples()` 반복 호출

- **문서:** `docs/phase4.md` "시료 이름·재고 조회 헬퍼: SampleController::instance().findSampleById(job.sampleId)" — `findSampleById` 메서드 사용 권장
- **코드:** `Semi/View/ProductionLineView.cpp:41~45, 78~81` — `sampleCtrl.getAllSamples()`로 전체 목록을 매 Job마다 반복 조회
- **차이:** `SampleController`에 `findSampleById` 메서드가 없어 계획 문서의 헬퍼 메서드가 미구현 상태. 기능은 동일하나 성능 측면에서 비효율적

### [MINOR-3] `ensureDbDir()`가 중첩 디렉터리 생성 불가

- **문서:** `docs/phase2.md` "DB 폴더 자동 생성: `<filesystem>` 또는 Windows API `CreateDirectoryA` 사용"
- **코드:** `Semi/Model/DataStore.cpp:24~29` — `CreateDirectoryA(dir.c_str(), nullptr)` 단일 호출
- **차이:** `CreateDirectoryA`는 부모 디렉터리가 없으면 실패함. 초기 배포 환경에서는 실패할 수 있음

### [MINOR-4] `SampleController`에 `instance()` 싱글톤 메서드 미구현

- **문서:** `docs/phase4.md` ProductionLineView 구현 요점: "sample = SampleController::instance().findSampleById(job.sampleId)" — SampleController를 싱글톤으로 사용 제안
- **코드:** `Semi/Controller/SampleController.h` — `instance()` 정적 메서드 없음, `ProductionLineView.cpp:24`에서 스택 인스턴스로 사용
- **차이:** 기능적으로 동일하며 SampleController가 상태를 갖지 않으므로 런타임 문제없음

---

### [AMBIGUOUS-1] `DataStore::load()`에서 JSON 키 접근 방식의 예외 안전성

- **위치:** `Semi/Model/DataStore.cpp:54~55`
- **모호한 이유:** `nlohmann::json::operator[]`은 키가 없을 때 null 값을 삽입하고 반환하지만, `.get<std::string>()`이 null에 호출되면 `nlohmann::json::type_error` 예외를 던짐. `docs/phase5.md`는 파일 파싱 오류에 대한 graceful 처리를 기술하지만, 파싱 성공 후 개별 키 접근 실패는 별개의 예외임
- **권고:** `.value("customer", std::string{})` 또는 `o.contains("customer") ? o["customer"].get<std::string>() : ""` 방식의 방어적 키 접근 사용

### [AMBIGUOUS-2] `onJobComplete()`에서 재고 음수 방지 로직의 충분성

- **위치:** `Semi/Controller/ProductionController.cpp:69~70`
- **모호한 이유:** `docs/phase3.md`에서 "수학적으로 보장"이라고 설명하나, 음수 방어 발동 시 재고가 0으로 설정되어 일부 생산분이 손실될 수 있음. 실제 발동 조건은 매우 제한적이지만 완전히 배제할 수 없음

### [AMBIGUOUS-3] 백그라운드 스레드 종료 시 진행 중인 작업 처리

- **위치:** `Semi/Controller/ProductionController.cpp:17~19`
- **모호한 이유:** `running_ = false`로 설정 후 `join()`을 기다리는 동안 최대 1초의 지연이 발생함. stop() 호출 시점에 `onJobComplete`가 실행 중이면 DataStore에 부분 업데이트가 완료될 때까지 join()이 대기함. 이 동작이 의도적인지 문서에 명시되지 않음

### [AMBIGUOUS-4] `workerLoop`에서 동일 Job의 중복 완료 처리 가능성

- **위치:** `Semi/Controller/ProductionController.cpp:27~32`
- **모호한 이유:** for 루프는 `getProductionJobs()`가 반환한 로컬 복사본을 순회하므로, 동일 사이클에 여러 Job이 완료 조건을 만족할 경우 재고 계산이 순차적이 아닌 동시 접근을 가정한 값으로 계산될 수 있음

---

### [INFO-1] `DataStore.h`에 테스트 전용 `resetForTest()` 메서드 추가

- `Semi/Model/DataStore.h:33~42` — `#ifdef SEMI_TEST` 조건부 컴파일로 `resetForTest()` 메서드 구현. 단위 테스트(`Tests/` 폴더)를 지원하기 위한 합리적 추가

### [INFO-2] `ProductionController::onJobComplete()`에서 재고 음수 방어 코드 추가

- `Semi/Controller/ProductionController.cpp:70` — `if (s2.stock < 0) s2.stock = 0;`. 문서에 명시되지 않은 방어 코드이나 합리적인 추가

### [INFO-3] `ProductionLineView`에서 플랫폼별 `localtime` 안전 처리

- `Semi/View/ProductionLineView.cpp:10~14` — `#ifdef _WIN32`로 `localtime_s`와 `localtime_r`를 분기 처리. 문서는 `localtime(t)` 직접 사용으로 기술되었으나 코드는 스레드 안전한 버전으로 개선됨

---

## 미구현 파일 목록

`docs/plan.md` 기준 기대 파일 26개 **모두 존재**. 미구현 파일 없음.

| 파일 | 존재 여부 |
|------|-----------|
| `Semi/main.cpp` | ✅ |
| `Semi/Lib/json.hpp` | ✅ |
| `Semi/Model/Sample.h` | ✅ |
| `Semi/Model/Order.h` | ✅ |
| `Semi/Model/Order.cpp` | ✅ |
| `Semi/Model/ProductionJob.h` | ✅ |
| `Semi/Model/DataStore.h` | ✅ |
| `Semi/Model/DataStore.cpp` | ✅ |
| `Semi/Controller/SampleController.h/.cpp` | ✅ |
| `Semi/Controller/OrderController.h/.cpp` | ✅ |
| `Semi/Controller/ProductionController.h/.cpp` | ✅ |
| `Semi/Controller/MonitoringController.h/.cpp` | ✅ |
| `Semi/Controller/ShippingController.h/.cpp` | ✅ |
| `Semi/View/MainMenuView.h/.cpp` | ✅ |
| `Semi/View/SampleView.h/.cpp` | ✅ |
| `Semi/View/OrderView.h/.cpp` | ✅ |
| `Semi/View/MonitoringView.h/.cpp` | ✅ |
| `Semi/View/ShippingView.h/.cpp` | ✅ |
| `Semi/View/ProductionLineView.h/.cpp` | ✅ |

---

## 검사 항목별 통과 결과 요약

| 검사 항목 | 결과 | 비고 |
|-----------|------|------|
| Sample 구조체 필드 일치 | ✅ 통과 | PRD 5개 필드 완전 일치 |
| Order 구조체 필드 일치 | ✅ 통과 | customer, createdAt, updatedAt 정상 추가 |
| ProductionJob 구조체 필드 일치 | ✅ 통과 | shortfall, startTime 마커 정상 구현 |
| OrderStatus 열거형 일치 | ✅ 통과 | 5개 상태 모두 구현 |
| Reserved→Confirmed 전이 | ✅ 통과 | approveOrder 재고 충분 분기 |
| Reserved→Producing 전이 | ✅ 통과 | approveOrder 재고 부족 분기 |
| Producing→Confirmed 전이 | ✅ 통과 | onJobComplete 내 구현 |
| Confirmed→Release 전이 | ✅ 통과 | processShipping 구현 |
| Reserved 거절 시 삭제 | ✅ 통과 | removeOrder 호출 확인 |
| 메인 메뉴 텍스트 일치 | ✅ 통과 | plan.md, PRD와 일치 |
| 시료관리 서브메뉴 | ✅ 통과 | 3개 항목 일치 |
| 주문관리 서브메뉴 | ✅ 통과 | 2개 항목 일치 |
| 모니터링 서브메뉴 | ✅ 통과 | 2개 항목 일치 |
| 출고처리 메뉴 | ✅ 통과 | 단일 화면 |
| 생산라인 메뉴 | ✅ 통과 | 활성/대기 표시 |
| 생산량 계산 ceil(shortfall/(yield*0.9)) | ✅ 통과 | OrderController.cpp:51 |
| 수율 적용 ceil(qty*yield) | ✅ 통과 | ProductionController.cpp:58 |
| 재고 차감(부족분만) | ✅ 통과 | ProductionController.cpp:69 |
| 재고 판단 stock>=quantity | ✅ 통과 | OrderController.cpp:42 |
| 재고 상태 분류(여유/부족/고갈) | ✅ 통과 | MonitoringController.cpp:28~31 |
| 거절 시 DataStore 삭제 | ✅ 통과 | removeOrder 호출 |
| addSample 즉시 save | ✅ 통과 | saveUnlocked 호출 패턴 |
| addOrder/updateOrder 즉시 save | ✅ 통과 | saveUnlocked 호출 패턴 |
| removeOrder 즉시 save | ✅ 통과 | saveUnlocked 호출 패턴 |
| DB 경로 설정 | ✅ 통과 | GetModuleFileName 기반 ../DB/data.json |
| load() 파일 없을 때 graceful | ✅ 통과 | `if (!ifs.is_open()) return;` |
| atomic write(임시파일→rename) | ✅ 통과 | MoveFileExA 사용 |
| JSON 필드명 스키마 일치 | ✅ 통과 | 코드 직렬화 필드명 일치 |
| 별도 스레드 생산 실행 | ✅ 통과 | std::thread worker_ |
| start()/stop() 순서 | ✅ 통과 | main.cpp 올바른 순서 |
| running_=false + join() | ✅ 통과 | stop() 구현 |
| FIFO 단일 활성 Job 정책 | ✅ 통과 | workerLoop hasActive 체크 |
| 완료 처리 후 신규 시작 | ✅ 통과 | 완료 처리 후 재조회하여 hasActive 판단 |
| 파일 구조 완전성 | ✅ 통과 | 26/26 파일 존재 |

---

## 권고 사항 (우선순위순)

### 1순위 — 즉시 수정 필요

**DB/data.json 데이터 정합성 수정**  
현재 `DB/data.json`의 orders 배열에 `customer` 필드가 없고, productionJobs 배열에 `shortfall` 필드가 없음. 이 파일로 프로그램을 시작하면 DataStore::load()에서 키 접근 예외가 발생함.
- 방법 1: `DB/data.json`을 올바른 스키마로 재작성 (customer, shortfall 필드 추가)
- 방법 2: `DataStore.cpp`의 load()에서 `o["customer"]` 대신 `o.value("customer", std::string{})` 방식으로 방어적 파싱으로 교체 (더 근본적인 해결책)

### 2순위 — 안정성 개선

**DataStore::load() 방어적 키 접근 적용**  
`Semi/Model/DataStore.cpp`의 load()에서 orders와 productionJobs 파싱 시 모든 키를 `.value("키명", 기본값)` 형식으로 교체하여 누락 필드에 대한 예외 발생을 방지할 것.

**ShippingView::show() 리팩토링**  
`Semi/View/ShippingView.cpp`에서 `getConfirmedOrders()` 이중 호출을 제거하고 한 번 조회한 결과를 재사용하도록 수정할 것. 백그라운드 스레드와의 경쟁 조건 창을 최소화하고 DataStore 잠금 오버헤드를 줄임.

### 3순위 — 품질 향상

**ensureDbDir() 중첩 디렉터리 지원**  
`Semi/Model/DataStore.cpp:24~29`의 `ensureDbDir()`을 `std::filesystem::create_directories()` 또는 재귀 `CreateDirectoryA` 호출로 교체하여 초기 배포 환경에서도 안전하게 생성되도록 수정할 것.

**productionJobs 동시 완료 처리 안전성 검토**  
`ProductionController::workerLoop()`에서 동일 사이클에 여러 Job이 완료될 때 재고 계산이 직렬화됨을 확인하고, 필요 시 배치 업데이트 API 도입을 검토할 것.

---

## 참고 파일 경로

| 파일 | 관련 이슈 |
|------|-----------|
| `Semi/Model/DataStore.cpp` (load() L31~75, saveUnlocked() L77~134) | MAJOR-1, MAJOR-2, AMBIGUOUS-1 |
| `Semi/Controller/OrderController.cpp` (approveOrder L32~71) | 통과 항목 |
| `Semi/Controller/ProductionController.cpp` (workerLoop L22~49, onJobComplete L52~79) | MAJOR-3, AMBIGUOUS-2,4 |
| `Semi/View/ShippingView.cpp` (show() L6~21) | MAJOR-3 |
| `DB/data.json` | MAJOR-1, MAJOR-2 |
