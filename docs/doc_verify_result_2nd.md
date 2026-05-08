# 문서 정합성 검사 결과

## 요약

- 검사 항목: 28개 (A~G 전 카테고리)
- 이상 없음 (통과): 19개
- 불일치/누락: 6개
- 계획 측 확장 (요구사항 외 추가): 3개

---

## 상세 결과

### [통과] A-1. Sample 속성 정합성

SemiPRD.md 정의: ID, 이름, 평균생산시간, 수율, 재고 (data structure 절).  
plan.md 및 phase2.md의 `Sample` 구조체: `id`, `name`, `avgProductionTimeSec`, `yield`, `stock` — 5개 속성 완전 반영.

---

### [통과] A-2. Order 속성 정합성

SemiPRD.md 정의: 번호, 고객, 시료, 수량, 상태.  
plan.md 및 phase2.md의 `Order` 구조체: `orderId`, `customer`, `sampleId`, `quantity`, `status` — 5개 속성 완전 반영. plan.md에 "Order.customer — 없음 → 추가 (PRD: 명시 필드)"로 변경 근거도 명시.

---

### [계획 측 확장] A-3. Order 타임스탬프 필드 추가

- 계획 내용: `Order` 구조체에 `createdAt`, `updatedAt` 필드를 추가 (plan.md, phase2.md).
- 평가: 합리적 확장. 요구사항(SemiPRD.md)에 타임스탬프 필드 명시는 없으나, 주문 처리 시간 추적과 ID 생성(`ORD-{timestamp}-{counter}`) 등 시스템 구현에 필요한 필드이며 요구사항과 충돌하지 않음.

---

### [통과] B-1. OrderStatus 5가지 상태 반영

SemiPRD.md 정의: Reserved, Rejected, Producing, Confirmed, Release.  
phase2.md `OrderStatus` 열거형에 5가지 상태 모두 포함. orderStatusToString / orderStatusFromString 직렬화 함수도 완비.

---

### [통과] B-2. 주문 상태 전이 흐름 정합성

SemiPRD.md 흐름: 접수 → Reserved → (승인: 재고 확인) → Confirmed 또는 Producing → Confirmed → (출고) → Release / (거절) → 즉시 삭제.  
phase3.md `OrderController` 구현 요점에 동일 흐름 반영: `approveOrder()` 내 재고 분기, `rejectOrder()` 내 `removeOrder()` 호출.

---

### [불일치] B-3. 재고 부족 시 처리 범위 — 승인 시 현재 재고 차감 누락

- 요구사항 (SemiPRD.md): "재고가 부족하다면 Producing 상태로 변경하고 부족분만큼 생산을 시작."
- 계획 내용 (phase3.md `approveOrder()`): 재고 부족 분기에서 `order.status = Producing`으로만 변경하고 현재 재고(stock)를 차감하지 않음. 생산량 산출 식은 `ceil(부족분 / (yield × 0.9))`이고 부족분은 `order.quantity - sample.stock`으로 올바르게 계산하지만, 생산 중 현재 재고를 선점(차감)하지 않아 다른 주문 승인 시 동일 재고가 중복 사용될 수 있음.
- 판정 근거: 요구사항은 "부족분만큼" 생산을 명시하므로, 기존 재고를 즉시 선점하는 처리가 필요하다. phase3.md에는 이 처리가 없고 `onJobComplete()`에서만 재고 차감이 이루어짐. 재고 선점 여부에 대한 명시적 설계 결정도 없음.

---

### [불일치] B-4. onJobComplete() 수율 적용 후 Confirmed 전환 논리

- 요구사항 (SemiPRD.md): "시료의 생산량은 부족분/(수율×0.9)으로 정해진다" → 수식에 의해 producedQty가 부족분을 충족하도록 설계.
- 계획 내용 (phase3.md `onJobComplete()`): `producedQty = floor(job.quantity * sample.yield)`로 수율 적용 후 "producedQty >= 부족분 이 보장된다"고 주석 처리. 그러나 단계 4에서 `sample.stock -= order.quantity` 후 Confirmed 전환할 때, 실제 stock이 order.quantity보다 부족한 경우(수율 오차 등)에도 무조건 Confirmed 처리하는 방어 로직이 없음.
- 판정 근거: 수율 수식으로 이론상 충족되나, 부동소수점 오차 또는 동시 다수 주문 상황에서 stock이 부족한 채 Confirmed 처리될 수 있음. phase3.md에 "이 시나리오는 허용 범위"라는 명시적 설계 결정이 없어 잠재 런타임 결함으로 분류.

---

### [통과] C-1. Main menu 5개 항목 정합성

SemiPRD.md: 시료관리, 주문(접수/승인/거절), 모니터링, 출고처리, 생산라인.  
plan.md 메뉴 구조 및 phase4.md `MainMenuView` 출력: 1.시료 관리, 2.주문(접수/승인/거절), 3.모니터링, 4.출고 처리, 5.생산 라인 — 완전 일치.

---

### [통과] C-2. 시료관리 서브메뉴 정합성

SemiPRD.md: 시료등록, 시료조회, 시료검색 3개 서브메뉴.  
phase4.md `SampleView`: 1.시료 등록, 2.시료 조회, 3.시료 검색 — 완전 일치.

---

### [불일치] C-3. 시료검색 기능 범위 축소

- 요구사항 (SemiPRD.md): "이름 등 속성으로 특정 시료를 검색."
- 계획 내용 (phase3.md `SampleController`): `searchByName(keyword)`만 구현 — 이름 검색에만 한정.
- 판정 근거: "이름 등 속성"이라는 표현은 이름 이외의 속성(예: ID)으로도 검색 가능함을 시사한다. 계획은 이름 검색만 구현하여 요구사항 범위를 축소함. plan.md에도 "이름 부분 검색"으로만 기술. ID 검색 등 다른 속성 검색 미반영.

---

### [통과] C-4. 주문(접수/승인/거절) 서브메뉴 정합성

SemiPRD.md: Reserved 상태 주문 출력, 승인/거절 선택 제공, 동작 후 display update.  
phase4.md `OrderView`: Reserved 목록 표시 → 승인/거절 선택 구조 반영. 서브메뉴에 주문 접수(주문담당자)와 승인/거절(생산담당자) 역할 레이블도 명시.

---

### [누락] C-5. 승인/거절 후 display update 미반영

- 요구사항 출처: SemiPRD.md "주문(접수/승인/거절)" 절 — "승인/거절 동작이 되면 display update."
- 문제: phase4.md `doReviewOrders()`는 승인/거절 처리 후 "승인 완료." 또는 "거절 완료." 메시지 출력으로 처리 종료. 승인/거절 후 화면을 갱신하여 갱신된 Reserved 목록을 재표시하는 동작이 계획에 기술되어 있지 않음. plan.md에도 해당 동작에 대한 언급 없음.

---

### [통과] C-6. 모니터링 서브메뉴 정합성

SemiPRD.md: 주문량확인(상태별 목록), 재고량확인(시료별 재고 + 여유/부족/고갈).  
phase3.md `MonitoringController` 및 phase4.md `MonitoringView`: 2개 서브메뉴 완전 반영. 상태 레이블(여유/부족/고갈)도 일치.

---

### [통과] C-7. 출고처리 메뉴 정합성

SemiPRD.md: CONFIRMED 상태 주문에 대해 출고 실행.  
phase3.md `ShippingController::processShipping()` 및 phase4.md `ShippingView`: Confirmed 상태 확인 후 Release 전환 — 완전 반영.

---

### [누락] C-8. 생산라인 "대기중인 생산큐" 확인 기능 누락

- 요구사항 출처: SemiPRD.md "Main menu > 5. 생산라인 : 현재 생산중인 시료 및 대기중인 생산큐 확인."
- 문제: plan.md는 "현재 생산 중인 Job 목록 + 남은 시간"만 기술. phase4.md `ProductionLineView`도 활성 Job 목록과 남은 시간만 출력하며 대기 큐 출력 없음. phase3.md `ProductionController`에도 대기 상태와 활성 상태를 구분하는 큐 구조가 없고 `getActiveJobs()`만 존재. "대기중인 생산큐" 기능이 plan/phase 어느 문서에도 구현 계획이 없음.

---

### [통과] D-1. 백그라운드 생산 처리 반영

CLAUDE.md: "시료의 생산은 background로 진행된다."  
plan.md: "백그라운드 스레드(ProductionController)가 생산 큐를 1초 간격으로 폴링." phase3.md `ProductionController::workerLoop()`으로 완전 반영.

---

### [통과] D-2. 상태 변경 시 JSON 저장 반영

CLAUDE.md: "주문, 시료의 상태가 변할 때마다 Data를 json파일로 저장."  
phase2.md `DataStore`: 모든 `add*/update*/remove*` 메서드 내부에서 `saveUnlocked()` 즉시 호출 — 상태 변경 즉시 저장 패턴 완전 반영.

---

### [통과] D-3. 실제 시간(초) 기반 동작 반영

CLAUDE.md: "시스템 내 시간은 실제 시간(초)을 기준으로 동작한다."  
plan.md 및 phase3.md: `std::time_t`, `std::time(nullptr)` 사용. `durationSec = avgProductionTimeSec × 생산량`으로 실제 초 단위 대기 처리. workerLoop 1초 간격 폴링 — 완전 반영.

---

### [통과] D-4. 재고 우선 전달 흐름 반영

CLAUDE.md: "재고가 있다면 바로 전달, 재고가 없다면 생산하여 전달."  
phase3.md `approveOrder()`: `sample.stock >= order.quantity`이면 Confirmed, 부족이면 Producing + 생산 Job 등록 — 완전 반영.

---

### [통과] E-1. 3개 Role 정합성

CLAUDE.md: 주문담당자, 생산담당자, 생산라인.  
phase4.md `OrderView` 서브메뉴 출력에 "주문 접수 (주문담당자)", "주문 승인/거절 (생산담당자)" 레이블 명시. 생산라인 역할은 `ProductionController` 백그라운드 스레드로 구현.

---

### [통과] E-2. Role별 책임 정합성

CLAUDE.md: 주문접수=주문담당자, 승인/거절=생산담당자.  
phase4.md `doCreateOrder()`는 주문담당자 역할, `doReviewOrders()`는 생산담당자 역할로 메뉴 레이블 및 기능 구분 일치.

---

### [통과] F-1. DB 폴더 JSON 저장 경로 정합성

CLAUDE.md: "json 파일들은 솔루션 내 DB 폴더에 저장."  
plan.md: "데이터 파일: `DB/data.json` (솔루션 루트 기준)." phase2.md `dbFilePath()`: `GetModuleFileNameA`로 실행파일 경로 취득 후 `../DB/data.json` 반환 — 완전 반영.

---

### [통과] F-2. 저장 트리거 정합성

CLAUDE.md: 상태 변경 시마다 저장.  
phase2.md: `addSample`, `updateSample`, `addOrder`, `updateOrder`, `removeOrder`, `addProductionJob`, `removeProductionJob` 모두 내부에서 `saveUnlocked()` 호출 패턴 — 완전 반영.

---

### [계획 측 확장] G-1. ProductionJob 모델 추가

- 계획 내용: plan.md, phase2.md에 `ProductionJob` 구조체(`jobId`, `orderId`, `sampleId`, `quantity`, `startTime`, `durationSec`) 및 `DB/data.json`의 `productionJobs` 배열 도입.
- 평가: 합리적 확장. SemiPRD.md에 ProductionJob이라는 별도 구조체는 명시되지 않았으나, 생산 큐 관리 및 백그라운드 생산 완료 판정을 위해 필요한 설계 결정. 요구사항의 생산 흐름과 충돌하지 않음.

---

### [계획 측 확장] G-2. 주문 ID 생성 방식 명시

- 계획 내용: plan.md, phase3.md에 `"ORD-{timestamp}-{counter}"` 형식의 주문 ID 생성 방식과 `atomic<int>` 카운터로 동일 초 충돌 방지 기술.
- 평가: 합리적 확장. SemiPRD.md에 주문 ID 생성 방식 미명시. 콘솔 프로그램 구현에 필요한 설계 세부사항으로 요구사항과 충돌하지 않음.

---

## 이슈 우선순위 요약

| 우선순위 | 유형 | 항목 |
|----------|------|------|
| 1 (High) | 누락 | C-8: 생산라인 "대기중인 생산큐" 확인 기능 — SemiPRD.md 명시 기능 미구현 |
| 2 (High) | 불일치 | B-3: 승인 시 재고 부족 분기에서 현재 재고 선점(차감) 처리 누락 |
| 3 (Medium) | 불일치 | B-4: onJobComplete() 수율 적용 후 stock 부족 시 방어 로직 미기술 |
| 4 (Medium) | 불일치 | C-3: 시료검색을 이름 검색만으로 축소 ("이름 등 속성" 요구 미반영) |
| 5 (Low) | 누락 | C-5: 승인/거절 후 display update(목록 갱신) 동작 미기술 |
| 6 (Low) | 불일치 | B-2 관련: plan.md 메뉴 구조에 거절 시 삭제 동작 미기술 (phase3.md에는 기술됨) |
