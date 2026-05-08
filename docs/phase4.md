# Phase 4 — View 레이어

## 목표
사용자 입출력을 담당하는 6개 View를 구현한다.  
View는 Controller를 호출하고 결과를 콘솔에 출력하는 역할만 한다.  
비즈니스 로직은 Controller에 위임한다.

---

## 파일 목록

| 파일 | 역할 |
|------|------|
| `MainMenuView.h / .cpp` | 메인 메뉴 루프 |
| `SampleView.h / .cpp` | 시료 등록·조회·검색 |
| `OrderView.h / .cpp` | 주문 접수·승인·거절 |
| `MonitoringView.h / .cpp` | 주문량·재고량 확인 |
| `ShippingView.h / .cpp` | 출고 처리 |
| `ProductionLineView.h / .cpp` | 생산 중인 Job 목록 |

---

## 4-1. MainMenuView

### 헤더

```cpp
#pragma once

class MainMenuView {
public:
    void run();
private:
    void printMenu() const;
    void dispatch(int choice);
};
```

### 콘솔 출력

```
========= 반도체 시료 관리 시스템 =========
1. 시료 관리
2. 주문 (접수 / 승인 / 거절)
3. 모니터링
4. 출고 처리
5. 생산 라인
0. 종료
선택:
```

### 구현 요점

```
run():
    while choice != 0:
        printMenu()
        cin >> choice
        dispatch(choice)
    출력: "프로그램을 종료합니다."

dispatch():
    1 → SampleView{}.show()
    2 → OrderView{}.show()
    3 → MonitoringView{}.show()
    4 → ShippingView{}.show()
    5 → ProductionLineView{}.show()
    0 → (루프 종료)
    else → "잘못된 입력입니다."
```

---

## 4-2. SampleView

### 헤더

```cpp
#pragma once

class SampleView {
public:
    void show();
private:
    void printMenu() const;
    void doRegister();
    void doList() const;
    void doSearch() const;
};
```

### 콘솔 출력 — 서브 메뉴

```
--- 시료 관리 ---
1. 시료 등록
2. 시료 조회
3. 시료 검색
0. 뒤로
선택:
```

### 시료 조회 테이블 형식

```
ID          이름                생산시간(초)  수율    재고
------------------------------------------------------------
S001        GaN Wafer           10            0.90    50
S002        SiC Substrate       15            0.85    0
```

### 구현 요점

#### doRegister()
```
입력: 시료 ID / 이름 / 평균 생산시간(초) / 수율(0.0~1.0)
SampleController::registerSample() 호출
성공 → "등록 완료."
실패(중복) → "이미 존재하는 ID입니다."
```

#### doSearch()
```
입력: 검색 키워드
SampleController::searchByName() 호출
결과 있으면 목록 출력, 없으면 "결과 없음."
```

---

## 4-3. OrderView

### 헤더

```cpp
#pragma once

class OrderView {
public:
    void show();
private:
    void printMenu() const;
    void doCreateOrder();
    void doReviewOrders();
};
```

### 콘솔 출력 — 서브 메뉴

```
--- 주문 관리 ---
1. 주문 접수 (주문담당자)
2. 주문 승인 / 거절 (생산담당자)
0. 뒤로
선택:
```

### 구현 요점

#### doCreateOrder()
```
입력: 고객명 / 시료 ID / 수량
OrderController::createOrder(customer, sampleId, quantity) 호출
성공 → "주문 접수 완료. 주문번호: {orderId}"
실패 → "존재하지 않는 시료 ID입니다."
```

**POC 대비 변경:** 고객명 입력 추가.

#### doReviewOrders()
```
OrderController::getReservedOrders() 호출
목록 없으면 → "승인 대기 중인 주문이 없습니다."
목록 있으면 Reserved 주문 테이블 출력:

[Reserved 주문 목록]
주문번호                  고객명          시료ID          수량
------------------------------------------------
ORD-1715000000-0          삼성전자        S001            5
...

입력: 처리할 주문번호 (취소: 0)
입력: 1.승인  2.거절

승인 → approveOrder() → "승인 완료." / "처리 실패."
거절 → rejectOrder()  → "거절 완료." / "처리 실패."
```

**POC 대비 변경:** 목록 테이블에 고객명 컬럼 추가.

---

## 4-4. MonitoringView

### 헤더

```cpp
#pragma once

class MonitoringView {
public:
    void show();
private:
    void printMenu() const;
    void doOrderCount() const;
    void doInventory() const;
};
```

### 콘솔 출력 — 서브 메뉴

```
--- 모니터링 ---
1. 주문량 확인
2. 재고량 확인
0. 뒤로
선택:
```

### 주문량 확인 출력 형식

```
[상태별 주문 현황]
Reserved    : 2건
  - ORD-1715000000-0  시료: S001  고객: 삼성전자  수량: 5
  - ORD-1715000001-1  시료: S002  고객: SK하이닉스  수량: 3
Producing   : 1건
  - ORD-1714999000-0  시료: S001  고객: LG전자  수량: 10
Confirmed   : 0건
Release     : 1건
  - ORD-1714990000-0  시료: S002  고객: 인텔  수량: 2
```

### 재고량 확인 출력 형식

```
[시료별 재고 현황]
ID          이름                재고    상태
------------------------------------------------
S001        GaN Wafer           50      여유
S002        SiC Substrate       0       고갈
```

- 여유(Sufficient) / 부족(Shortage) / 고갈(Depleted)

### 구현 요점

```cpp
static const char* stockStatusLabel(StockStatus s) {
    switch (s) {
        case StockStatus::Sufficient: return "여유";
        case StockStatus::Shortage:   return "부족";
        case StockStatus::Depleted:   return "고갈";
        default:                      return "-";
    }
}
```

---

## 4-5. ShippingView

### 헤더

```cpp
#pragma once

class ShippingView {
public:
    void show();
private:
    void printConfirmedOrders() const;
};
```

### 콘솔 출력 형식

```
--- 출고 처리 ---
주문번호                  고객명          시료ID          수량
------------------------------------------------
ORD-1715000100-0          삼성전자        S001            5

출고할 주문번호 (취소: 0):
```

### 구현 요점

```
1. printConfirmedOrders(): Confirmed 목록 출력
2. 목록 없으면 "출고 가능한 주문이 없습니다." 출력 후 반환
3. 주문번호 입력 (0이면 취소)
4. ShippingController::processShipping() 호출
   성공 → "출고 완료."
   실패 → "처리 실패 (Confirmed 상태가 아니거나 존재하지 않는 주문)."
```

**POC 대비 변경:** 목록 테이블에 고객명 컬럼 추가.

---

## 4-6. ProductionLineView

### 헤더

```cpp
#pragma once

class ProductionLineView {
public:
    void show() const;
};
```

### 콘솔 출력 형식

```
--- 생산 라인 ---
Job ID             주문번호                  시료ID        수량    남은시간(초)
-----------------------------------------------------------------------------
JOB-ORD-1715-0     ORD-1715000000-0          S001          10      45
```

### 구현 요점

```
jobs = ProductionController::instance().getActiveJobs()
now = time(nullptr)
for job in jobs:
    remaining = max(0, job.durationSec - (now - job.startTime))
    출력 한 행
```

남은 시간이 0인 Job은 다음 workerLoop 사이클에서 완료 처리된다.  
표시상 0초로 출력하고, 실제 완료는 백그라운드에서 이루어진다.

---

## 공통 구현 패턴

### 입력 처리

```cpp
// 숫자 메뉴 선택 후 개행 문자 제거
std::cin >> choice;
std::cin.ignore();

// 문자열 입력
std::getline(std::cin, str);
```

`cin >> 숫자` 후 반드시 `cin.ignore()`를 호출해야  
다음 `getline`에서 빈 문자열이 읽히는 문제를 방지할 수 있다.

### 테이블 정렬

```cpp
#include <iomanip>
std::cout << std::left << std::setw(25) << orderId
          << std::setw(16) << customer
          << std::setw(12) << sampleId
          << std::setw(8)  << quantity << "\n";
```

---

## 완료 기준

- 메인 메뉴에서 0~5 선택 정상 동작, 범위 외 입력 시 오류 메시지 출력
- 시료 등록 후 시료 조회에서 즉시 확인 가능
- 주문 접수 → 승인 → 출고까지 전체 플로우 콘솔에서 실행 가능
- 재고 없는 시료 승인 시 Producing으로 전환되고 생산 라인 메뉴에 표시됨
- 생산 완료(durationSec 경과) 후 자동 Confirmed 전환, 출고 처리 메뉴에 표시됨
