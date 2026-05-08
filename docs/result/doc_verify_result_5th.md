# 문서 정합성 검사 결과 (5차)

**검사 일자:** 2026-05-08
**검사 대상:** `CLAUDE.md`, `SemiPRD.md`, `docs/plan.md`, `docs/phase1.md` ~ `phase5.md`
**참조:** `docs/doc_verify_result_4th.md` (4차)

---

## 4차 대비 개선 확인 요약

4차 검사에서 발견된 불일치/누락/요구사항 결함 총 8개 중 **7개가 개선** 완료되었습니다. 잔존 불일치는 1개입니다.

| 4차 항목 | 내용 | 5차 판정 |
|---|---|---|
| P4-1 | 생산라인 현재 생산 중 컬럼 불일치 | 개선됨 |
| P4-2 | 대기 큐 3개 표시 제한 누락 | 개선됨 |
| P4-3 | 대기 큐 출력 컬럼 불일치 | 개선됨 |
| P3-1 | plan.md ProductionController "POC와 동일" 오기 (3-3절) | 개선됨 |
| N-1 | SemiPRD.md 17번 줄 미완성 문장 | 개선됨 |
| N-2 | 시료 속성 수 모순 (4가지 vs 5가지) | 개선됨 |
| N-3 | "preducing" 오탈자 | 개선됨 |
| N-4 | data structure 절 Rejected 누락 | 개선됨 |
| **P3-1(잔존)** | plan.md Phase 4 ProductionLineView "POC와 동일" 오기 (166번 줄) | **잔존** |

---

## 요약

| 구분 | 건수 |
|---|---|
| 검사 항목 | 34개 |
| 이상 없음(통과) | 28개 |
| 잔존 불일치 | 1개 |
| 계획 측 확장(합리적) | 4개 |
| 요구사항 문서 내 자체 결함 | 0개 (4차 4건 모두 수정) |

---

## 상세 결과

### [불일치 - 낮음] P3-1(잔존): plan.md Phase 4 — ProductionLineView "POC와 동일" 오기

- **요구사항:** SemiPRD.md 48~49번 줄 — 생산라인 메뉴는 현재 생산 중 6개 컬럼(주문번호/시료이름/주문량/재고/필요 생산량/완료 예정 시간)과 대기 큐 3개 및 6개 컬럼(순서/주문번호/시료이름/주문량/필요 생산량/완료 예정 시간) 출력을 요구함
- **계획 내용 (plan.md 166번 줄):** `"6. ProductionLineView — POC와 동일"` 로 기술됨
- **실제 내용 (phase4.md 4-6절):** POC 대비 실질적 변경이 발생함. 시료이름·재고 조회 헬퍼 추가, `formatTime()` 함수 추가, 현재 생산 중 6컬럼 출력, 대기 큐 최대 3개 및 6컬럼 출력, 누적 완료 예정 시간 계산 로직(`nextStart` 누적) 포함
- **판정 근거:** plan.md Phase 4 요약의 ProductionLineView 항목이 phase4.md 실제 구현 계획과 불일치. "POC와 동일"은 변경 없음을 의미하나, ProductionLineView는 SemiPRD.md 요구사항 충족을 위해 POC 대비 상당한 변경이 이루어짐. plan.md를 단독으로 읽는 독자에게 잘못된 정보를 전달할 수 있음.

---

### [통과] A-1: Sample 구조체 5개 속성 완전 반영

SemiPRD.md의 시료 5개 속성(ID, 이름, 평균생산시간, 수율, 재고)이 plan.md 및 phase2.md `Sample` 구조체(`id`, `name`, `avgProductionTimeSec`, `yield`, `stock`)에 모두 반영됨.

### [통과] A-2: Order 구조체 속성 완전 반영

SemiPRD.md의 주문 속성(번호, 고객, 시료, 수량, 상태)이 plan.md 및 phase2.md `Order` 구조체(`orderId`, `customer`, `sampleId`, `quantity`, `status`)에 모두 반영됨. POC에서 누락된 `customer` 필드를 명시적으로 추가하고 근거를 기술함.

### [통과] B-1: OrderStatus 5가지 상태 완전 반영

Reserved, Rejected, Producing, Confirmed, Release 5가지가 phase2.md `OrderStatus` 열거형과 직렬화 함수에 모두 반영됨.

### [통과] B-2: 주문 상태 전이 흐름 정합성

SemiPRD.md의 주문 처리 순서(Reserved → 재고 유무 분기 → Confirmed/Producing → Confirmed → Release, 거절 시 즉시 삭제)가 plan.md 및 phase3.md OrderController/ProductionController에 정확히 반영됨.

### [통과] B-3: 재고 선점 처리 반영

재고 부족 승인 시 `sample.stock = 0` 즉시 차감 로직이 phase3.md approveOrder()에 명시되어 중복 재고 사용 방지가 보장됨.

### [통과] B-4: 생산 완료 수율 적용 및 ceil 보장

phase3.md onJobComplete()에서 `ceil(job.quantity * sample.yield)`를 적용하여 부동소수점 오차로 인한 shortfall 미달을 수학적으로 방지하는 근거가 명시됨.

### [통과] C-1: 메인 메뉴 5개 항목 정합성

SemiPRD.md Main menu 5개 항목이 plan.md 및 phase4.md MainMenuView에 모두 반영됨.

### [통과] C-2: 시료관리 서브메뉴 3개 정합성

시료등록/시료조회/시료검색 3개 서브메뉴가 phase4.md SampleView에 정확히 반영됨.

### [통과] C-3: 시료검색 4속성 검색 기능 반영

plan.md 메뉴 구조의 "ID / 이름 / 평균생산시간 / 수율, 부분 문자열 일치" 상세 기술과 phase3.md SampleController `SampleSearchField` 열거형, phase4.md SampleView doSearch() 4속성 선택 메뉴가 일치함.

### [통과] C-4: 주문 승인/거절 역할 레이블 정합성

phase4.md OrderView 서브메뉴에 "(주문담당자)" / "(생산담당자)" 레이블이 명시되어 CLAUDE.md 역할 구분과 일치함.

### [통과] C-5: display update 루프 재진입 방식 반영

SemiPRD.md의 "승인/거절 동작이 되면 display update" 요건이 phase4.md doReviewOrders() while 루프 재진입으로 구현됨.

### [통과] C-6: 모니터링 서브메뉴 및 재고 상태 3단계 정합성

주문량확인/재고량확인 2개 서브메뉴와 여유/부족/고갈 3단계, Reserved+Producing 합산 기준이 phase3.md MonitoringController 및 phase4.md MonitoringView에 정확히 반영됨.

### [통과] C-7: 출고처리 Confirmed → Release 정합성

SemiPRD.md의 confirmed 주문 출고 및 release 상태 변경이 phase3.md ShippingController 및 phase4.md ShippingView에 정확히 반영됨.

### [통과] C-8: 생산 대기 큐 확인 기능 반영

phase3.md `getPendingJobs()` 및 phase4.md ProductionLineView "[생산 대기 큐]" 섹션으로 반영됨.

### [통과] C-9: 생산라인 현재 생산 중 출력 6컬럼 완전 반영 (4차 P4-1 해소)

SemiPRD.md 48번 줄의 6컬럼(주문번호/시료이름/주문량/재고/필요 생산량/완료 예정 시간)이 phase4.md ProductionLineView 콘솔 출력 형식 및 구현 요점에 모두 반영됨. 시료이름·재고 헬퍼, `formatTime()`, `job.shortfall` 매핑이 명시됨.

### [통과] C-10: 생산 대기 큐 3개 제한 및 6컬럼 완전 반영 (4차 P4-2, P4-3 해소)

SemiPRD.md 49번 줄의 3개 제한과 6컬럼(순서/주문번호/시료이름/주문량/필요 생산량/완료 예정 시간)이 phase4.md의 `pendingJobs[:3]` 슬라이스 및 `nextStart` 누적 계산 로직으로 반영됨.

### [통과] D-1: 백그라운드 생산 스레드 반영

CLAUDE.md의 "생산은 background로 진행" 요건이 plan.md 아키텍처 설명 및 phase3.md ProductionController workerLoop()에 완전히 반영됨.

### [통과] D-2: 상태 변경 즉시 JSON 저장 반영

CLAUDE.md의 "상태 변할 때마다 json 파일로 저장" 요건이 phase2.md DataStore의 `saveUnlocked()` 패턴(모든 변경 메서드 내부 즉시 호출)으로 반영됨.

### [통과] D-3: 실제 시간(초) 기반 동작 반영

CLAUDE.md의 "시스템 내 시간은 실제 시간(초) 기준" 요건이 `std::time_t`, `std::time(nullptr)`, 1초 간격 폴링, `durationSec` 단위(초)로 반영됨.

### [통과] D-4: 재고 우선 전달 흐름 반영

CLAUDE.md의 "재고 있으면 바로 전달, 없으면 생산 후 전달" 흐름이 phase3.md approveOrder() 분기로 정확히 반영됨.

### [통과] E-1: 3개 Role 정합성 및 역할별 책임 분리

CLAUDE.md의 주문담당자/생산담당자/생산라인 역할이 phase4.md OrderView 레이블 및 역할별 기능 분리로 반영됨.

### [통과] E-2: FIFO 단일 생산 정책 반영

SemiPRD.md의 FIFO 방식 처리 요건이 plan.md "FIFO 단일 생산 정책" 절 및 phase3.md workerLoop() hasActive/firstPending 분기로 반영됨.

### [통과] F-1: DB 폴더 경로 정합성

CLAUDE.md의 "DB 폴더에 저장" 요건이 plan.md `DB/data.json` 및 phase2.md `dbFilePath()` 구현으로 반영됨.

### [통과] F-2: 저장 트리거 정합성

모든 add*/update*/remove* 메서드 내부에서 `saveUnlocked()`를 즉시 호출하는 패턴이 phase2.md에 명시됨.

### [통과] F-3: Atomic write 구현 반영

phase2.md `saveUnlocked()`에 임시파일 방식 및 `MoveFileExA(MOVEFILE_REPLACE_EXISTING)` 원자적 교체가 명시됨.

### [통과] G-1: 주문 삭제 정책 정합성

SemiPRD.md의 Rejected 즉시 삭제 요건이 plan.md 및 phase3.md rejectOrder()에 반영됨. Release 이력 보존 정책도 명시됨.

### [통과] SemiPRD.md 자체 결함 4종 수정 확인 (4차 N-1, N-2, N-3, N-4 해소)

| 4차 항목 | 내용 | 5차 현황 |
|---|---|---|
| N-1 | 17번 줄 미완성 문장 | 수정 완료 |
| N-2 | 9번 줄 "4가지" vs 53번 줄 5개 모순 | 수정 완료 (현재 "5가지 속성") |
| N-3 | "preducing" 오탈자 | 수정 완료 |
| N-4 | data structure 절 Rejected 누락 | 수정 완료 (현재 53번 줄에 Rejected 포함) |

---

## 계획 측 확장 (합리적)

| 항목 | 내용 | 평가 |
|---|---|---|
| X-1 | Order 타임스탬프 (createdAt, updatedAt) | 합리적 확장. 이력 추적에 유용하며 요구사항과 충돌 없음 |
| X-2 | ProductionJob 독립 모델 및 productionJobs DB 배열 | 합리적 확장. 생산 상태 영속화에 필수 |
| X-3 | 주문 ID 생성 방식 (ORD-{timestamp}-{counter}) | 합리적 확장. 고유성 보장을 위한 설계 세부사항 |
| X-4 | 싱글톤 DataStore + thread-safe 뮤텍스 + saveUnlocked 분리 | 합리적 확장. 백그라운드 스레드 안전성을 위한 필수 설계 결정 |

---

## 우선순위 요약

| 우선순위 | 심각도 | 구분 | 항목 | 대상 문서 |
|---|---|---|---|---|
| 1 | 낮음 | 불일치 | P3-1(잔존): plan.md 166번 줄 ProductionLineView "POC와 동일" 오기 | `docs/plan.md` |

---

## 최우선 수정 권고

**P3-1 (낮음)** — `docs/plan.md` 166번 줄을 다음과 같이 수정 권고:

```
6. `ProductionLineView` — 시료이름·재고 조회 헬퍼 추가; 현재 생산 중 6컬럼 출력(주문번호/시료이름/주문량/재고/필요생산량/완료 예정 시간); 대기 큐 최대 3개 6컬럼 출력(순서/주문번호/시료이름/주문량/필요생산량/완료 예정 시간); 누적 완료 예정 시간 계산 로직 추가
```
