# 문서 정합성 검사 결과

**검사 일자:** 2026-05-08  
**검사 대상:** CLAUDE.md, SemiPRD.md, docs/plan.md, docs/phase1~5.md

---

## 요약

- 검사 항목: 24개
- 이상 없음: 14개 (통과)
- 불일치/누락: 5개

---

## 이슈 목록

| # | 심각도 | 유형 | 항목 |
|---|--------|------|------|
| 1 | Warning | 불일치 | `phase3.md` `onJobComplete()` — 수율 적용 후 재고 부족 시 무조건 Confirmed 처리하는 논리 결함 |
| 2 | Warning | 불일치 | `Rejected` 열거형이 `Order` 구조체에 존재하지만 실제 DB에는 Rejected 상태 레코드가 저장되지 않는 개념 모순 |
| 3 | Warning | 불일치 | `phase3.md` `MonitoringController` — 활성 주문 범위를 `Reserved+Producing`으로 해석하였으나 요구사항에 명시 없음 |
| 4 | Warning | 누락 | 생산라인 "대기중인 생산큐" 확인 기능이 `plan.md` / `phase4.md` 어디에도 구현 계획 없음 |
| 5 | Info | 누락 | `plan.md` 메뉴 구조에 거절 시 삭제 동작이 미기술 (`phase3.md`에는 기술됨) |

---

## 상세 내용

### [이슈 1] Warning — onJobComplete() 수율 적용 후 논리 결함

- **출처:** `docs/phase3.md` — `ProductionController::onJobComplete()`
- **문제:** 의사코드 4번 단계에서 `sample.stock < order.quantity`인 경우에도 `if` 블록 바깥에서 무조건 `order.status = Confirmed`로 전환한다. 수율 적용 후 생산량이 주문 수량보다 적을 경우(예: quantity=10, yield=0.5이면 생산량=5), 시료를 충분히 받지 못한 주문이 Confirmed 처리된다.
- **권장 수정:** `phase3.md` `onJobComplete()` 의사코드를 수정하여 `stock < quantity`인 경우의 처리(재생산 대기 또는 부분 출고 등)를 명확히 기술하거나, 요구사항이 수율로 인한 부족 상황을 고려하지 않는다고 명시적으로 선언해야 한다.

---

### [이슈 2] Warning — Rejected 열거형 개념 모순

- **출처:** `SemiPRD.md` "주문(접수/승인/거절)", `docs/phase2.md` Order.h
- **문제:** `SemiPRD.md`는 "Rejected: 주문이 거절된 상태. 더이상 관리하지 않고 주문 목록에서 삭제"라고 정의하지만, `phase2.md`의 `OrderStatus` 열거형에는 `Rejected`가 여전히 포함되어 있다. 거절된 주문은 DB에 Rejected 상태로 남지 않고 즉시 삭제되므로, 열거형의 `Rejected` 값이 실제로는 사용되지 않는다.
- **권장 수정:** 계획 문서에 "Rejected는 즉시 삭제되므로 DB에 Rejected 상태의 Order는 존재하지 않는다"는 점을 명시하거나, 열거형에서 `Rejected`를 제거하는 방향을 검토해야 한다.

---

### [이슈 3] Warning — 재고량확인 활성 주문 기준 미명시

- **출처:** `SemiPRD.md` "모니터링 > 재고량확인", `docs/phase3.md` `MonitoringController::getInventoryStatus()`
- **문제:** 요구사항은 "주문대비 수량"을 기준으로 한다고 명시하지만, 활성 주문 범위(Reserved만 포함인지 Reserved+Producing 포함인지)를 명시하지 않았다. 계획은 Reserved+Producing을 포함하도록 확장 해석하였다.
- **권장 수정:** "활성 주문이 없고 stock > 0인 경우 여유로 표시됨"과 "활성 주문 기준은 Reserved+Producing"임을 `phase3.md`에 명시적으로 기술하면 완결성이 높아진다.

---

### [이슈 4] Warning — 생산라인 "대기중인 생산큐" 기능 누락

- **출처:** `SemiPRD.md` "Main menu > 5. 생산라인", `docs/plan.md`, `docs/phase4.md`
- **문제:** `SemiPRD.md`는 "현재 생산중인 시료 및 대기중인 생산큐 확인"을 요구하지만, `plan.md`는 "현재 생산 중인 Job 목록 + 남은 시간"만 기술한다. `phase4.md` `ProductionLineView`도 활성 Job 목록과 남은 시간만 표시하며, 대기 큐에 대한 언급이 없다. 현재 설계에서는 생산 Job이 생성된 즉시 시작 시간이 설정되므로 모든 Job이 "현재 생산 중"으로 취급되어 대기 큐가 실질적으로 발생하지 않는다.
- **권장 수정:** `phase3.md` `ProductionController`에 대기 상태(`pendingJobs`)와 활성 상태(`activeJobs`)를 구분하는 큐 구조를 추가하거나, 요구사항의 "대기중인 생산큐"는 현재 구조에서 발생하지 않는다는 설계 결정을 `plan.md`에 명시해야 한다.

---

### [이슈 5] Info — plan.md 거절 시 삭제 동작 미기술

- **출처:** `docs/plan.md` 메뉴 구조, `docs/phase3.md` `OrderController::rejectOrder()`
- **문제:** `plan.md` 메뉴 구조 설명에는 승인 시 처리만 기술되어 있고, 거절 시 `DataStore::removeOrder(orderId)` 호출로 삭제됨이 명시되지 않았다. `phase3.md`에는 기술되어 있으므로 plan.md와의 일관성이 부족하다.
- **권장 수정:** `plan.md` 메뉴 구조에 "거절 시: Rejected 처리 후 주문 목록에서 즉시 삭제"를 추가한다.

---

## 가장 시급한 수정 대상

1. **이슈 #1** — 수율로 인해 생산량이 주문 수량보다 적을 경우에도 Confirmed 처리 → 런타임 버그로 이어질 수 있음.
2. **이슈 #4** — `SemiPRD.md`가 명시한 "대기중인 생산큐 확인" 기능이 계획에서 누락됨.

---

## 통과 항목 목록

| 항목 | 내용 |
|------|------|
| A-1 | Sample 속성 정합성 (id, name, avgProductionTimeSec, yield, stock) |
| A-2 | Order 속성 정합성 (orderId, customer, sampleId, quantity, status) |
| B-1 | 주문 5가지 상태 열거형 반영 (Reserved, Rejected, Producing, Confirmed, Release) |
| B-2 | 주문 상태 전이 흐름 정합성 |
| C-1 | 메인 메뉴 5개 항목 정합성 |
| C-2 | 시료관리 서브메뉴 정합성 (등록/조회/검색) |
| C-3 | 모니터링 서브메뉴 정합성 (주문량확인/재고량확인) |
| D-1 | 백그라운드 생산 처리 (별도 스레드 workerLoop) |
| D-2 | 상태 변경 시 JSON 저장 (saveUnlocked() 즉시 호출) |
| D-3 | 실제 시간(초) 기반 동작 (std::time_t, std::time(nullptr)) |
| D-4 | 재고 우선 전달 흐름 (stock 비교 후 분기) |
| E-1 | 3개 Role 정합성 (주문담당자, 생산담당자, 생산라인) |
| F-1 | DB 폴더 JSON 저장 경로 정합성 (../DB/data.json) |
| G-1 | plan.md ↔ phase1~5 전체 범위 일치 |
