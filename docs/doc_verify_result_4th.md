# 문서 정합성 검사 결과 (4차)

**검사 일자:** 2026-05-08
**검사 대상:** CLAUDE.md, SemiPRD.md, docs/plan.md, docs/phase1~5.md
**참조:** docs/doc_verify_result.md (1차), docs/doc_verify_result_2nd.md (2차), docs/doc_verify_result_3rd.md (3차)

---

## 요약

| 구분 | 건수 |
|------|------|
| 검사 항목 | 34개 |
| 통과 | 24개 |
| 3차 대비 개선 확인 | 5개 |
| 잔존 불일치/누락 | 4개 |
| 신규 발견 | 1개 |
| 계획 측 확장(합리적) | 4개 |
| 요구사항 문서 내 자체 결함(잔존) | 3개 |

---

## 1. 3차 검토 대비 개선 확인된 항목

| 3차 이슈 | 내용 | 판정 |
|----------|------|------|
| R-4 | FIFO 단일 생산 규칙 미반영 | plan.md에 "FIFO 단일 생산 정책" 절 추가, phase3.md workerLoop에 hasActive/firstPending 분기 로직 추가로 해결 |
| R-1 | plan.md ProductionJob shortfall 누락 | plan.md 구조체 정의에 `shortfall` 필드 및 설명 추가, DB 스키마 예시에도 반영으로 해결 |
| R-2 | plan.md 메뉴 구조 미갱신 | plan.md 메뉴 구조에 시료검색 4속성, 생산 대기 큐 목록 반영으로 해결 |
| R-3 | 역할 명칭 불일치 | 실제 확인 결과 SemiPRD.md 24번 줄이 "생산담당자"를 사용하므로 CLAUDE.md와 일치. 불일치 해소 |
| R-5 | atomic write 채택 여부 미명시 | phase2.md `saveUnlocked()`에 임시파일 방식 및 MoveFileExA 구현 코드 추가로 해결, plan.md 구현 단계에도 적용 기술 |

---

## 2. 잔존 불일치/누락 항목

### [불일치 - 높음] P4-1: 생산라인 "현재 생산 중" 출력 컬럼 불일치

- **요구사항 (SemiPRD.md 48번 줄):** "현재 생산중인 주문을 출력. 주문번호, 시료이름, 주문량, 재고, 필요 생산량, 완료 예정 시간."
- **계획 내용 (phase4.md 4-6 ProductionLineView):** "Job ID, 주문번호, 시료ID, 수량, 남은시간(초)" 5개 컬럼 출력.
- **판정 근거:**
  - "재고" 컬럼 누락: 요구사항은 현재 재고 수치를 함께 표시해야 하나, 계획에는 없음.
  - "필요 생산량" 컬럼 누락: 요구사항은 필요 생산량을 표시해야 하나, 계획에는 없음. `ProductionJob.quantity`(실제 생산 수량)로 대체 가능하나 명시적 매핑이 없음.
  - "시료이름" vs "시료ID": 요구사항은 시료 이름 표시를 요구하나, 계획은 시료 ID만 출력. 시료 이름 조회를 위한 DataStore 접근 로직도 계획에 없음.
  - "완료 예정 시간" vs "남은시간(초)": 요구사항은 완료 예정 시간(절대 시각 또는 시각 문자열)을 요구하나, 계획은 잔여 초를 출력. 의미상 유사하지만 표현 방식이 다름.
  - "Job ID" 추가: 요구사항에 없는 컬럼이 추가됨 (계획 측 확장으로 분류 가능하나, 요구사항 컬럼 누락과 동시에 발생하므로 불일치로 판정).

### [불일치 - 중간] P4-2: 생산라인 "대기 큐" 3개 표시 제한 누락

- **요구사항 (SemiPRD.md 49번 줄):** "대기중인 주문 실행 순서대로 **3개** 출력."
- **계획 내용 (phase4.md 4-6 ProductionLineView):** 대기 Job 전체를 FIFO 순서대로 출력하며, 3개 제한 없음.
- **판정 근거:** 요구사항이 대기 큐 표시를 명시적으로 3개로 제한하고 있으나, 계획에서 이 제한이 누락되어 모든 대기 Job을 출력하도록 설계됨. 이는 요구사항 범위를 초과하는 동시에 명세와 다른 동작을 유발함.

### [불일치 - 중간] P4-3: 생산라인 "대기 큐" 출력 컬럼 불일치

- **요구사항 (SemiPRD.md 49번 줄):** "순서, 주문번호, 시료이름, 주문량, 필요 생산량, 완료 예정 시간."
- **계획 내용 (phase4.md 4-6 ProductionLineView):** "Job ID, 주문번호, 시료ID, 수량" 4개 컬럼 출력.
- **판정 근거:**
  - "순서" 컬럼 누락: 대기 큐에서의 처리 순서를 명시하는 컬럼이 없음.
  - "시료이름" vs "시료ID": 동일한 문제. 요구사항은 이름, 계획은 ID 사용.
  - "필요 생산량" 컬럼 누락.
  - "완료 예정 시간" 컬럼 누락: 대기 중인 주문의 완료 예정 시간을 표시해야 하나 계획에 없음. (대기 중이므로 앞선 Job 처리 시간 합산이 필요하여 구현이 복잡하나 요구사항에 명시됨)

### [불일치 - 낮음] P3-1: plan.md Phase 3 설명에 "ProductionController — POC와 동일" 기술 부정확

- **계획 내용 (plan.md Phase 3):** `"ProductionController — POC와 동일 (백그라운드 스레드)"`
- **실제 내용 (phase3.md):** POC 대비 FIFO 단일 생산 정책, `getActiveJobs()` / `getPendingJobs()` 분리, `updateProductionJob()` 추가 등 상당한 변경이 이루어짐.
- **판정 근거:** plan.md 구현 단계 설명이 phase3.md 실제 내용과 다르게 기술되어 있음. 다른 Phase 항목(OrderController, OrderView 등)은 "POC 대비 변경" 사항을 명시했으나, ProductionController는 변경 내용이 큰데도 "POC와 동일"로 오기되어 있어 문서 간 불일치.

---

## 3. 신규 발견 이슈

### [요구사항 결함 - 낮음] N-4: SemiPRD.md data structure 절 주문 상태 목록에 Rejected 미포함

- **출처:** SemiPRD.md 53번 줄 — `"주문 : 번호, 고객, 시료, 수량, 상태(Reserved, Producing, Confirmed, Release)."`
- **문제:** data structure 절의 주문 상태 열거는 4개(Reserved, Producing, Confirmed, Release)이나, 시스템 구성 요소 절 2번에서는 5가지 상태(Reserved, Rejected, Producing, Confirmed, Release)로 정의하고 있음. data structure 절에서 Rejected 상태가 누락되어 두 정의 간 내부 불일치가 존재함.
- **영향:** 구현 계획(phase2.md)은 시스템 구성 요소 절의 5가지 상태를 채택하여 올바르게 구현됨. 요구사항 문서 자체의 결함이므로 구현 계획에 대한 지시는 없으나, 향후 문서 유지보수 시 혼란 가능성 있음.

---

## 4. 요구사항 문서 내 자체 결함 (잔존)

3차에서 발견된 이하 항목은 수정이 이루어지지 않아 잔존함.

| 항목 | 내용 | 심각도 |
|------|------|--------|
| N-1 | SemiPRD.md 17번 줄 미완성 문장: "승인이 된다면 재고 확인하여 다음 동작 결정.(재고의 확인은" — 괄호 미닫힘, 문장 끊김 | 높음 |
| N-2 | SemiPRD.md 9번 줄 "4가지 속성" vs 53번 줄 5개 속성 모순 | 낮음 |
| N-3 | SemiPRD.md 19번 줄 "preducing" 오탈자 (올바른 표기: "Producing") | 낮음 |

---

## 5. 통과 항목

| 항목 | 내용 |
|------|------|
| A-1 | Sample 5개 속성 완전 반영 (id, name, avgProductionTimeSec, yield, stock) |
| A-2 | Order 5개 속성 완전 반영 (orderId, customer, sampleId, quantity, status) |
| B-1 | OrderStatus 5가지 열거형 완전 반영 및 직렬화 함수 완비 |
| B-2 | 주문 상태 전이 흐름 정합성 (Reserved→Confirmed/Producing→Confirmed→Release, 거절→즉시 삭제) |
| B-3 | 재고 선점 처리: approveOrder 재고 부족 시 stock=0 즉시 차감 반영 |
| B-4 | onJobComplete ceil 적용 및 수학적 보장 주석 반영 |
| C-1 | 메인 메뉴 5개 항목 정합성 (시료관리/주문/모니터링/출고처리/생산라인) |
| C-2 | 시료관리 서브메뉴 3개 정합성 (등록/조회/검색) |
| C-3 | 시료검색 4속성 검색 (ID/이름/평균생산시간/수율) 반영 |
| C-4 | 주문 승인/거절 서브메뉴 및 역할 레이블 정합성 |
| C-5 | display update 루프 재진입 방식 반영 |
| C-6 | 모니터링 2개 서브메뉴 정합성 (주문량확인/재고량확인 + 여유/부족/고갈) |
| C-7 | 출고처리 Confirmed→Release 정합성 |
| C-8 | 생산 대기 큐 확인 기능 반영 (getPendingJobs + startTime=0 마커) |
| D-1 | 백그라운드 생산 스레드 반영 (ProductionController workerLoop) |
| D-2 | 상태 변경 즉시 JSON 저장 (saveUnlocked 패턴) |
| D-3 | 실제 시간(초) 기반 동작 (std::time_t, std::time(nullptr)) |
| D-4 | 재고 우선 전달 흐름 (stock 비교 분기: Confirmed/Producing) |
| E-1 | 3개 Role 정합성 및 역할별 책임 분리 (주문담당자/생산담당자/생산라인) |
| E-2 | FIFO 단일 생산 정책 plan.md 및 phase3.md 반영 |
| F-1 | DB 폴더 경로 정합성 (../DB/data.json, GetModuleFileNameA) |
| F-2 | 저장 트리거 정합성 (모든 add*/update*/remove* 내부 saveUnlocked 호출) |
| F-3 | Atomic write (임시파일 → MoveFileExA rename) 구현 반영 |
| G-1 | 주문 삭제 정책 plan.md 명시 (거절 시 즉시 삭제, Release 이력 보존) |

---

## 6. 계획 측 확장 (합리적)

| 항목 | 내용 | 평가 |
|------|------|------|
| X-1 | Order 타임스탬프 필드 (createdAt, updatedAt) | 합리적 확장. 이력 추적에 필요하며 요구사항과 충돌 없음. |
| X-2 | ProductionJob 독립 모델 및 productionJobs DB 배열 | 합리적 확장. 백그라운드 생산 관리 및 재시작 후 상태 복원에 필수. |
| X-3 | 주문 ID 생성 방식 (ORD-{timestamp}-{counter}) | 합리적 확장. 동일 초 중복 방지를 위한 설계 세부사항. |
| X-4 | Job ID 컬럼 생산라인 출력에 추가 | 합리적 확장. 운영/디버그 편의를 위한 추가이나, 요구사항 컬럼 미반영 문제가 더 큰 우선 과제. |

---

## 7. 우선순위 요약

| 우선순위 | 심각도 | 구분 | 항목 | 대상 문서 |
|----------|--------|------|------|-----------|
| 1 | 높음 | 불일치 | P4-1: 생산라인 현재 생산 중 출력 컬럼 불일치 (재고/필요 생산량/시료이름 누락) | phase4.md |
| 2 | 높음 | 요구사항 결함(잔존) | N-1: SemiPRD.md 17번 줄 미완성 문장 | SemiPRD.md |
| 3 | 중간 | 불일치 | P4-2: 대기 큐 3개 표시 제한 누락 | phase4.md |
| 4 | 중간 | 불일치 | P4-3: 대기 큐 출력 컬럼 불일치 (순서/시료이름/필요 생산량/완료 예정 시간 누락) | phase4.md |
| 5 | 낮음 | 불일치 | P3-1: plan.md ProductionController "POC와 동일" 오기 | plan.md |
| 6 | 낮음 | 신규(요구사항 결함) | N-4: data structure 절 주문 상태 목록 Rejected 누락 | SemiPRD.md |
| 7 | 낮음 | 요구사항 결함(잔존) | N-2: 시료 속성 수 모순 (4가지 vs 5가지) | SemiPRD.md |
| 8 | 낮음 | 요구사항 결함(잔존) | N-3: "preducing" 오탈자 | SemiPRD.md |

---

## 8. 최우선 수정 권고

1. **P4-1 (높음)** — `phase4.md` ProductionLineView 현재 생산 중 출력 컬럼을 SemiPRD.md 요구사항에 맞게 수정. "재고" 표시를 위해 DataStore/SampleController 접근 추가, "필요 생산량"은 `ProductionJob.shortfall` 또는 `quantity`로 매핑, "시료이름"은 sampleId로 시료 조회 후 name 표시, "완료 예정 시간"은 `startTime + durationSec`을 시각 문자열로 변환하는 로직 추가 필요.

2. **P4-2 / P4-3 (중간)** — `phase4.md` ProductionLineView 대기 큐 출력을 3개로 제한하고 "순서, 시료이름, 필요 생산량, 완료 예정 시간" 컬럼 추가. 완료 예정 시간은 앞선 활성 Job의 완료 예정 시각에 자신의 durationSec을 더하는 방식으로 산출 가능.

3. **N-1 (높음)** — `SemiPRD.md` 17번 줄 "(재고의 확인은" 이후 미완성 문장을 완성. "(재고의 확인은 승인 시점에 이루어진다)"와 같이 의도를 명확히 기술 권장.
