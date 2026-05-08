# Semi E2E 테스트 계획서

## 개요

Semi 콘솔 프로그램의 전체 흐름을 stdin 파이프로 자동 구동하는 E2E 테스트 계획이다.  
각 테스트 케이스는 **사전조건 → 입력 시퀀스 → 검증 항목** 3단계로 구성된다.

---

## 테스트 환경

| 항목 | 내용 |
|------|------|
| 실행 파일 | `x64/Debug/Semi.exe` (솔루션 루트 기준) |
| DB 파일 | `DB/data.json` |
| 빌드 방식 | `msbuild Semi/Semi.vcxproj /p:OutDir=<솔루션루트>\x64\Debug\` |
| 기준 시료 | S001(Silicon Wafer A, 생산5초, 수율0.92, 재고15) |
|           | S002(GaAs Wafer B, 생산3초, 수율0.85, 재고0) |
|           | S003(SiC Substrate C, 생산3초, 수율0.78, 재고3) |

### 생산량 계산식 (PRD 기준)

```
생산량    = ceil(부족분 / (수율 × 0.9))
완료재고  = ceil(생산량 × 수율) - 부족분 + 기존재고
생산시간  = 평균생산시간 × 생산량  [초]
```

---

## 콘솔 입력 표기 규칙

각 테스트 케이스의 입력 시퀀스는 아래 형식으로 표기한다.

```
[메뉴번호|입력값]  ← 입력 한 줄 = 엔터 1회
```

메뉴 구조 요약:
```
메인: 1=시료관리  2=주문  3=모니터링  4=출고처리  5=생산라인  0=종료
주문: 1=주문접수  2=승인/거절  0=뒤로
  주문접수: 고객명 → 시료ID → 수량
  승인/거절: 주문번호 → 1(승인)|2(거절)
시료관리: 1=등록  2=조회  3=검색  0=뒤로
  시료등록: 시료ID → 이름 → 평균생산시간 → 수율
모니터링: 1=주문량확인  2=재고량확인  0=뒤로
출고처리: 주문번호 입력
```

---

## 테스트 케이스

---

### TC-01: 재고 충분 → 즉시 Confirmed → 출고

**목적:** 재고가 주문량 이상일 때 승인 즉시 Confirmed, 출고 후 Release 전환 확인

**사전조건:**
```json
{ "orders": [], "productionJobs": [],
  "samples": [{"id":"S001","stock":15}, ...] }
```

**입력 시퀀스:**
```
2          ← 주문 메뉴
1          ← 주문 접수
TC01-Customer
S001
5
0          ← 주문 메뉴 뒤로
2          ← 주문 메뉴
2          ← 승인/거절
{orderId}  ← 생성된 주문번호 (검증 후 입력)
1          ← 승인
0          ← 종료
4          ← 출고 처리
{orderId}  ← 출고
0          ← 종료
```

> 자동화 시: `data.json`에서 orderId를 읽어 두 번째 실행에 입력한다.  
> 또는 한 번의 실행으로 처리하려면 주문 접수→승인→출고를 순서대로 이어 입력한다.

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | 콘솔 출력 | "주문 접수 완료. 주문번호: ORD-..." |
| 2 | 콘솔 출력 | "승인 완료." |
| 3 | 콘솔 출력 | "출고 완료." |
| 4 | `data.json` orders[0].status | `"Release"` |
| 5 | `data.json` samples[S001].stock | `10` (15 - 5) |

---

### TC-02: 재고 전무 → 전량 생산 후 Confirmed

**목적:** 재고 0일 때 전량 생산이 시작되고, 완료 후 Confirmed 전환 확인

**사전조건:**
```json
S002: stock=0, avgProductionTimeSec=60, yield=0.85
```

**생산량 계산:**
```
부족분    = 3 (qty=3, stock=0)
생산량    = ceil(3 / (0.85 × 0.9)) = ceil(3 / 0.765) = ceil(3.922) = 4
생산시간  = 3 × 4 = 12초
완료재고  = ceil(4 × 0.85) - 3 + 0 = ceil(3.4) - 3 = 4 - 3 = 1
```

**입력 시퀀스 (1단계 — 주문 접수 + 승인):**
```
2
1
TC02-Customer
S002
3
0
2
2
{orderId}
1
0
0
```

**검증 항목 (즉시):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | 콘솔 출력 | "승인 완료." |
| 2 | `data.json` orders[0].status | `"Producing"` |
| 3 | `data.json` productionJobs 길이 | `1` |
| 4 | productionJobs[0].quantity | `4` |
| 5 | productionJobs[0].shortfall | `3` |
| 6 | productionJobs[0].durationSec | `12` |

**검증 항목 (12초 대기 후):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 7 | `data.json` orders[0].status | `"Confirmed"` |
| 8 | `data.json` productionJobs 길이 | `0` |
| 9 | `data.json` samples[S002].stock | `1` |

---

### TC-03: 재고 부분 보유 → 부족분 생산

**목적:** 재고 일부 있을 때 부족분만 생산하고 나머지 재고는 즉시 소진

**사전조건:**
```json
S003: stock=3, avgProductionTimeSec=45, yield=0.78
```

**생산량 계산:**
```
부족분    = 6 - 3 = 3  (qty=6, stock=3)
생산량    = ceil(3 / (0.78 × 0.9)) = ceil(3 / 0.702) = ceil(4.274) = 5
생산시간  = 3 × 5 = 15초
완료재고  = ceil(5 × 0.78) - 3 + 0 = ceil(3.9) - 3 = 4 - 3 = 1
```

**입력 시퀀스 (주문 접수 + 승인):**
```
2
1
TC03-Customer
S003
6
0
2
2
{orderId}
1
0
0
```

**검증 항목 (즉시):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | orders[0].status | `"Producing"` |
| 2 | samples[S003].stock | `0` (기존 재고 즉시 소진) |
| 3 | productionJobs[0].shortfall | `3` |
| 4 | productionJobs[0].quantity | `5` |
| 5 | productionJobs[0].durationSec | `15` |

**검증 항목 (15초 대기 후):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 6 | orders[0].status | `"Confirmed"` |
| 7 | samples[S003].stock | `1` |

---

### TC-04: 재고 정확 일치 → stock이 0이 되는 승인

**목적:** 주문량 = 재고량일 때 승인 후 재고가 정확히 0이 되는지 확인

**사전조건:**
```json
S001: stock=15
```

**입력 시퀀스:**
```
2
1
TC04-Customer
S001
15
0
2
2
{orderId}
1
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | orders[0].status | `"Confirmed"` |
| 2 | samples[S001].stock | `0` |
| 3 | productionJobs 길이 | `0` (생산 없음) |

---

### TC-05: 주문 거절 → 주문 삭제 확인

**목적:** 거절 시 주문이 목록에서 완전 삭제되고 Rejected 상태로 남지 않음을 확인

**입력 시퀀스:**
```
2
1
TC05-Customer
S001
3
0
2
2
{orderId}
2
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | 콘솔 출력 | "거절 완료." |
| 2 | `data.json` orders 배열 길이 | `0` |
| 3 | 거절된 orderId로 findOrder | 존재하지 않음 |
| 4 | samples[S001].stock | 변화 없음 (재고 차감 없어야 함) |

---

### TC-06: 잘못된 시료 ID → 주문 접수 실패

**목적:** 존재하지 않는 시료 ID 입력 시 주문이 등록되지 않음을 확인

**입력 시퀀스:**
```
2
1
TC06-Customer
S999
5
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | 콘솔 출력 | "존재하지 않는 시료 ID입니다." |
| 2 | `data.json` orders 배열 길이 | `0` (주문 등록 없음) |

---

### TC-07: 새 시료 등록 후 주문

**목적:** 런타임에 등록한 신규 시료로 즉시 주문 가능한지 확인

**사전조건:**
```json
S004 없음
```

**입력 시퀀스:**
```
1          ← 시료 관리
1          ← 시료 등록
S004
Test Sample D
20
0.90
0          ← 시료 관리 뒤로
2          ← 주문
1          ← 주문 접수
TC07-Customer
S004
2
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | 콘솔 출력 | "등록 완료." |
| 2 | 콘솔 출력 | "주문 접수 완료. 주문번호: ORD-..." |
| 3 | samples 배열에 S004 존재 | `true` |
| 4 | S004.avgProductionTimeSec | `20` |
| 5 | orders[0].sampleId | `"S004"` |
| 6 | orders[0].status | `"Reserved"` |

---

### TC-08: 중복 시료 ID 등록 시도

**목적:** 이미 존재하는 ID로 시료 등록 시 실패 처리 확인

**입력 시퀀스:**
```
1
1
S001
Duplicate Sample
30
0.90
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | 콘솔 출력 | "이미 존재하는 ID입니다." |
| 2 | `data.json` samples 배열 길이 | 변화 없음 |
| 3 | S001의 이름 | `"Silicon Wafer A"` (덮어쓰이지 않음) |

---

### TC-09: FIFO 다중 생산 큐

**목적:** 동일 시료에 복수 주문 승인 시 FIFO 순서로 생산되는지 확인

**사전조건:**
```json
S002: stock=0, avgProductionTimeSec=60, yield=0.85
```

**생산량 계산:**
```
Job1: qty=2, 부족분=2, 생산량=ceil(2/0.765)=ceil(2.614)=3, 시간=3×3=9초
Job2: qty=1, 부족분=1, 생산량=ceil(1/0.765)=ceil(1.307)=2, 시간=3×2=6초
```

**입력 시퀀스:**
```
2
1
TC09-Customer-A
S002
2
1
TC09-Customer-B
S002
1
0
2
2
{orderId_A}
1
{orderId_B}
1
0
0
```

**검증 항목 (즉시):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | productionJobs 길이 | `2` |
| 2 | jobs 중 startTime > 0 인 것 | `1` (FIFO: Job1만 진행 중) |
| 3 | startTime == 0 인 Job | Job2 (대기 중) |

**검증 항목 (9초 후 — Job1 완료):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 4 | orderId_A 상태 | `"Confirmed"` |
| 5 | orderId_B 상태 | `"Producing"` |
| 6 | Job2의 startTime > 0 | `true` (자동 시작) |

**검증 항목 (추가 6초 후 — Job2 완료):**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 7 | orderId_B 상태 | `"Confirmed"` |
| 8 | productionJobs 길이 | `0` |

---

### TC-10: 재고 상태 모니터링 (여유/부족/고갈)

**목적:** Reserved/Producing 주문 합산 기준으로 재고 상태 3종 표시 정확성 확인

**사전조건:**
```json
S001: stock=5
S002: stock=0
S003: stock=10
```

**시나리오 구성:**
```
S001: stock=5, Reserved 주문 qty=3 존재  → demanded=3 < 5  → 여유(Sufficient)
S001: stock=5, Reserved 주문 qty=8 존재  → demanded=8 > 5  → 부족(Shortage)
S002: stock=0 (주문 없음)                → demanded=0, stock==0 → 고갈(Depleted)
S003: stock=10, 주문 없음                → demanded=0         → 여유(Sufficient)
```

**입력 시퀀스 (여유 케이스 확인):**
```
2
1
TC10-CustomerA
S001
3
0
3          ← 모니터링
2          ← 재고량 확인
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | S001 재고 상태 (Reserved qty=3 존재, stock=5) | `여유` |
| 2 | S002 재고 상태 (stock=0) | `고갈` |
| 3 | S001 추가 주문(qty=8) 후 모니터링 | `부족` |

**입력 시퀀스 (부족→고갈 전이 확인):**
```
2
1
TC10-CustomerB
S001
8
0
3
2
0
0
```

---

### TC-11: 생산 라인 화면 표시 확인

**목적:** 생산 중인 작업과 대기 큐가 올바르게 표시되는지 확인

**사전조건:** S002(stock=0) 주문 3건 승인 → 3개 ProductionJob 생성

**입력 시퀀스:**
```
2
1
TC11-A
S002
1
1
TC11-B
S002
1
1
TC11-C
S002
1
0
2
2
{orderId_A}
1
{orderId_B}
1
{orderId_C}
1
0
5          ← 생산 라인
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | "[현재 생산 중]" 섹션에 1개 표시 | startTime > 0인 Job |
| 2 | "[생산 대기 큐]" 섹션에 최대 2개 표시 (3개 중 대기 2) | Job2, Job3 |
| 3 | 완료 예정 시간 | 현재시각 + durationSec |

---

### TC-12: 연속 출고 처리

**목적:** Confirmed 주문이 여럿일 때 한 건씩 선택 출고 가능한지 확인

**사전조건:** S001(stock=10) 주문 2건 각각 Confirmed 상태

**입력 시퀀스:**
```
2
1
TC12-A
S001
3
1
TC12-B
S001
2
0
2
2
{orderId_A}
1
{orderId_B}
1
0
4          ← 출고 처리 (1건)
{orderId_A}
4          ← 출고 처리 (2건)
{orderId_B}
0
0
```

**검증 항목:**

| # | 검증 내용 | 기대값 |
|---|-----------|--------|
| 1 | orderId_A 상태 | `"Release"` |
| 2 | orderId_B 상태 | `"Release"` |
| 3 | 출고처리 화면에 더 이상 주문 없음 | "출고 가능한 주문이 없습니다." |

---

## 검증 방법

### 콘솔 출력 검증

`printf '...' | ./x64/Debug/Semi.exe` 결과를 문자열 매칭으로 검사한다.

```bash
output=$(printf '...' | ./x64/Debug/Semi.exe 2>&1)

# 주문 접수 성공 여부
echo "$output" | grep -c "주문 접수 완료"

# 승인 완료 여부
echo "$output" | grep -c "승인 완료"
```

### DB 상태 검증

실행 후 `DB/data.json`을 읽어 JSON 필드를 검사한다.

```bash
# Python으로 파싱
python3 -c "
import json
with open('DB/data.json') as f:
    d = json.load(f)
orders = d['orders']
print(f'주문 수: {len(orders)}')
for o in orders:
    print(f'  {o[\"orderId\"]}: {o[\"status\"]}')
"
```

### 생산 완료 대기

생산 완료가 필요한 케이스는 `durationSec + 2`초 sleep 후 DB를 재검사한다.

```bash
# TC-02 예시: 12초 대기
sleep 17
python3 -c "import json; d=json.load(open('DB/data.json')); print(d['orders'][0]['status'])"
```

---

## 실행 우선순위

| 우선순위 | 케이스 | 이유 |
|---------|--------|------|
| P1 | TC-01, TC-05, TC-06 | 핵심 Happy/Sad Path |
| P1 | TC-02, TC-03, TC-04 | 생산 로직 전체 검증 |
| P2 | TC-07, TC-08 | 시료 관리 기능 |
| P2 | TC-09 | FIFO 생산 큐 동작 |
| P3 | TC-10, TC-11, TC-12 | 모니터링/출고/생산라인 UI |

---

## 주의사항

- 각 TC 실행 전에 `DB/data.json`의 `orders`, `productionJobs`를 초기화한다 (`samples`는 유지).
- 생산 완료 검증(TC-02, TC-03, TC-04, TC-09)은 실제 시간이 소요되므로 자동화 시 타임아웃을 충분히 설정한다.
- TC-09 FIFO 검증은 Job1 완료(9초) 직후 DB를 즉시 읽어야 Job2 대기 상태를 포착할 수 있다.
- `Semi.exe`에 stdin을 파이프할 때 `orderId`처럼 런타임에 결정되는 값은 실행을 분리하거나 DB에서 읽어 두 번째 실행에 주입한다.
