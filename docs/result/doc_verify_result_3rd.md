# 문서 정합성 검사 결과 (3차)

**검사 일자:** 2026-05-08
**검사 대상:** CLAUDE.md, SemiPRD.md, docs/plan.md, docs/phase1~5.md
**참조:** docs/doc_verify_result.md (1차), docs/doc_verify_result_2nd.md (2차)

---

## 요약

| 구분 | 건수 |
|------|------|
| 검사 항목 | 32개 |
| 통과 | 19개 |
| 이전 검토 대비 개선 확인 | 5개 |
| 잔존 불일치/누락 | 5개 |
| 신규 발견 | 3개 |
| 계획 측 확장(합리적) | 3개 |
| 요구사항 문서 내 자체 결함 | 4개 |

---

## 1. 이전 검토(2차) 대비 개선 확인된 항목

| 2차 이슈 | 내용 | 판정 |
|----------|------|------|
| B-3 | 재고 부족 시 선점 처리 누락 | phase3.md에 `sample.stock = 0 + shortfall` 로직 추가로 해결 |
| B-4 | onJobComplete floor→수율 오차 | ceil 변경 및 수학적 보장 주석 추가로 해결 |
| C-3 | 시료 검색 이름 전용 축소 | `SampleSearchField` 열거형 4속성 + phase4.md 검색 메뉴 추가로 해결 |
| C-5 | 승인/거절 후 display update 미기술 | phase4.md while 루프 재진입 방식으로 해결, plan.md에도 반영 |
| C-8 | 대기중인 생산큐 기능 누락 | `getPendingJobs()`, `startTime=0` 마커, phase4.md `[생산 대기 큐]` 출력 추가로 해결 |

---

## 2. 잔존 이슈

### [불일치 - 높음] R-4: FIFO 단일 종류 생산 규칙 미반영

- **요구사항 (SemiPRD.md 13번 줄):** "생산은 한 종류의 시료씩 이루어지며 주문 입력 순서대로(FIFO방식으로) 처리한다."
- **계획 내용:** plan.md, phase3.md `workerLoop()` 어디에도 이 제약이 언급되지 않음.
- **현재 workerLoop 로직:**
  ```
  for job in jobs:
      if job.startTime == 0:
          job.startTime = now   // 대기 중인 모든 Job에 동시 startTime 부여
  ```
  대기 중인 Job이 복수일 때 동일 폴링 사이클에서 모두 `startTime = now`가 설정되어 병렬 생산이 시작된다. 요구사항의 "한 종류씩 순차 처리" 규칙에 정면으로 위배된다.
- **수정 방향:** workerLoop에서 활성 Job(`startTime > 0`)이 한 건도 없을 때만 대기 Job 목록의 첫 번째 항목 하나를 활성화하는 조건 분기를 추가해야 한다. plan.md에도 FIFO 정책을 명시해야 한다.

### [불일치 - 중간] R-1: plan.md ProductionJob 구조체가 phase2.md와 다름

- **plan.md 기술:**
  ```cpp
  struct ProductionJob {
      std::string jobId; std::string orderId; std::string sampleId;
      int quantity; std::time_t startTime; int durationSec;
  };
  ```
- **phase2.md 기술 (실제 구현 기준):** `shortfall` 필드 추가, `startTime = 0`을 대기 마커로 사용.
- **DB 스키마 예시 (plan.md):** `shortfall` 필드 없음, `startTime`이 양수값만 예시됨.
- **판정:** plan.md가 개요 문서이더라도 phase2.md 구현과 구조체 정의가 다르면 혼란을 초래한다. plan.md 갱신 필요.

### [불일치 - 낮음] R-2: plan.md 메뉴 구조 기술이 phase 단계의 실제 구현과 불일치

- **시료 검색:** plan.md "시료 검색 (이름 부분 검색)" — phase3/4에서 4속성 검색으로 확장됐으나 미반영.
- **생산 라인:** plan.md "현재 생산 중인 Job 목록 + 남은 시간" — phase4.md에서 `[생산 대기 큐]` 출력도 추가됐으나 미반영.

### [불일치 - 낮음] R-3: 승인/거절 담당 역할 명칭 불일치

- **CLAUDE.md:** "생산담당자"가 승인과 생산을 진행.
- **SemiPRD.md 24번 줄:** "생산라인 담당자의 승인·거절처리".
- **계획:** phase4.md는 "생산담당자"를 채택하여 CLAUDE.md를 따름.
- **문제:** 두 요구사항 문서가 서로 다른 용어를 사용하며 이들이 동일 역할인지 명시적 선언이 없음.

### [불일치 - 낮음] R-5: plan.md atomic write 언급이 phase2.md에서 결론 없이 누락

- **plan.md:** "save는 atomic write (임시파일 → rename) 고려"
- **phase2.md:** `saveUnlocked()`에서 `std::ofstream ofs(dbFilePath()); ofs << j.dump(2);` 직접 쓰기만 기술. 채택 여부 설명 없음.

---

## 3. 신규 발견 이슈

### [요구사항 결함 - 높음] N-1: SemiPRD.md 17번 줄 미완성 문장

- **출처:** `SemiPRD.md` 17번 줄
- **문제:** "승인이 된다면 재고 확인하여 다음 동작 결정.(재고의 확인은" — 괄호가 닫히지 않고 문장이 끊겨 있다. 재고 확인의 시점이나 기준이 명세되지 않은 채 2-1, 2-2 항목으로 넘어간다.
- **영향:** 구현 계획은 "승인 시점"에 재고를 즉시 확인한다고 해석하여 구현했으나, 요구사항 문서상 근거가 불완전하다.

### [요구사항 결함 - 낮음] N-2: SemiPRD.md 시료 속성 수 내부 모순

- **9번 줄:** "시료별로 **4가지** 속성(ID, 이름, 평균생산시간, 수율)을 가진다." — 재고 속성 누락.
- **45번 줄:** "시료 : ID, 이름, 평균생산시간, 수율, **재고**." — 5개 속성.
- **영향:** 구현 계획은 45번 줄을 따라 `stock` 포함 5개 속성으로 올바르게 구현됨.

### [요구사항 결함 - 낮음] N-3: SemiPRD.md "preducing" 오탈자

- **출처:** SemiPRD.md 19번 줄 — "preducing 상태로 변경"
- **올바른 표기:** "Producing" (SemiPRD.md 3번 줄에서 올바르게 정의됨)
- **영향:** 계획 문서는 "Producing"으로 올바르게 사용하므로 구현에 미치는 영향은 없음.

---

## 4. 계획 측 확장 (합리적)

| 항목 | 내용 | 평가 |
|------|------|------|
| E-1 | Order 타임스탬프 필드 (`createdAt`, `updatedAt`) — SemiPRD.md 미명시, plan.md/phase2.md에 추가 | 합리적 확장. 이력 추적에 필요. 요구사항과 충돌 없음. |
| E-2 | `ProductionJob` 독립 모델 및 `productionJobs` DB 배열 — SemiPRD.md 미명시 | 합리적 확장. 백그라운드 생산 관리 및 재시작 후 상태 복원에 필수. |
| E-3 | 주문 ID 생성 방식 (`ORD-{timestamp}-{counter}`, `atomic<int>` 카운터) — SemiPRD.md 미명시 | 합리적 확장. 요구사항과 충돌 없음. |

---

## 5. 통과 항목

| 항목 | 내용 |
|------|------|
| A-1 | Sample 5개 속성 완전 반영 (id, name, avgProductionTimeSec, yield, stock) |
| A-2 | Order 5개 속성 완전 반영 (orderId, customer, sampleId, quantity, status) |
| B-1 | OrderStatus 5가지 열거형 완전 반영 |
| B-2 | 주문 상태 전이 흐름 정합성 (Reserved→Confirmed/Producing→Confirmed→Release, 거절→즉시 삭제) |
| B-3 | 재고 선점 처리 (2차 개선, phase3.md 반영 확인) |
| B-4 | onJobComplete ceil 적용 및 수학적 보장 (2차 개선, phase3.md 반영 확인) |
| C-1 | 메인 메뉴 5개 항목 정합성 |
| C-2 | 시료관리 서브메뉴 3개 정합성 |
| C-3 | 시료 검색 4속성 확장 (2차 개선, phase3/4에서 반영 확인) |
| C-4 | 주문 승인/거절 서브메뉴 및 역할 레이블 정합성 |
| C-5 | display update (루프 재진입 방식) 반영 (2차 개선 확인) |
| C-6 | 모니터링 2개 서브메뉴 정합성 (주문량확인/재고량확인) |
| C-7 | 출고처리 Confirmed→Release 정합성 |
| C-8 | 생산 대기 큐 확인 기능 (2차 개선, phase3/4에서 반영 확인) |
| D-1 | 백그라운드 생산 스레드 반영 |
| D-2 | 상태 변경 즉시 JSON 저장 (saveUnlocked 패턴) |
| D-3 | 실제 시간(초) 기반 동작 (std::time_t) |
| D-4 | 재고 우선 전달 흐름 (stock 비교 분기) |
| E-1 | 3개 Role 정합성 및 역할별 책임 분리 |
| F-1 | DB 폴더 경로 및 저장 트리거 정합성 |

---

## 6. 우선순위 요약

| 우선순위 | 심각도 | 구분 | 항목 | 대상 문서 |
|----------|--------|------|------|-----------|
| 1 | 높음 | 불일치 | R-4: FIFO 단일 생산 규칙 미반영 — workerLoop 병렬 생산 버그 | phase3.md, plan.md |
| 2 | 높음 | 요구사항 결함 | N-1: SemiPRD.md 미완성 문장 (재고 확인 기준 불명확) | SemiPRD.md |
| 3 | 중간 | 불일치 | R-1: plan.md ProductionJob 구조체 shortfall 누락 | plan.md |
| 4 | 낮음 | 불일치 | R-2: plan.md 메뉴 구조 미갱신 (검색 4속성, 대기 큐) | plan.md |
| 5 | 낮음 | 불일치 | R-3: 역할 명칭 불일치 (생산담당자 vs 생산라인 담당자) | CLAUDE.md, SemiPRD.md |
| 6 | 낮음 | 불일치 | R-5: atomic write 채택 여부 미명시 | plan.md, phase2.md |
| 7 | 낮음 | 요구사항 결함 | N-2: 시료 속성 수 모순 (4가지 vs 5가지) | SemiPRD.md |
| 8 | 낮음 | 요구사항 결함 | N-3: "preducing" 오탈자 | SemiPRD.md |

---

## 최우선 수정 권고

1. **R-4 (높음)** — `phase3.md` `workerLoop()`를 수정하여 활성 Job(`startTime > 0`)이 없을 때만 대기 Job 목록의 첫 번째 하나를 활성화하도록 변경. plan.md에도 FIFO 및 "한 종류씩 생산" 정책을 명시해야 함.

2. **N-1 (높음)** — `SemiPRD.md` 17번 줄의 미완성 문장을 완성하거나, "(재고의 확인은 승인 시점에 이루어진다)"와 같이 의도를 명확히 기술.

3. **R-1 (중간)** — `plan.md` `ProductionJob` 구조체에 `shortfall` 필드 추가 및 `startTime = 0` 대기 마커 설명 추가. DB 스키마 예시에도 반영.
