# Phase 5 — 진입점 및 최종 통합

## 목표
`main.cpp`를 작성하고 전체 컴포넌트를 연결한다.  
프로그램 시작/종료 시 DataStore 복원·저장과 백그라운드 스레드 관리를 올바르게 처리한다.

---

## 5-1. main.cpp

```cpp
#include "Model/DataStore.h"
#include "Controller/ProductionController.h"
#include "View/MainMenuView.h"

int main() {
    // 이전 상태 복원 (DB/data.json → 메모리)
    DataStore::instance().load();

    // 백그라운드 생산 스레드 시작
    ProductionController::instance().start();

    // 메인 메뉴 루프 (0 선택 시 반환)
    MainMenuView{}.run();

    // 생산 스레드 정리 후 종료
    ProductionController::instance().stop();

    return 0;
}
```

POC와 동일. 변경 없음.

---

## 5-2. 전체 컴포넌트 연결 구조

```
main()
 │
 ├─ DataStore::load()          JSON → 메모리 복원
 │
 ├─ ProductionController::start()
 │    └─ workerLoop() [background thread]
 │         └─ 1초마다 ProductionJob 완료 여부 체크
 │              └─ onJobComplete() → DataStore update → save()
 │
 ├─ MainMenuView::run() [main thread]
 │    ├─ SampleView    → SampleController    → DataStore
 │    ├─ OrderView     → OrderController     → DataStore
 │    ├─ MonitoringView → MonitoringController → DataStore (읽기 전용)
 │    ├─ ShippingView  → ShippingController  → DataStore
 │    └─ ProductionLineView → ProductionController (읽기 전용)
 │
 └─ ProductionController::stop()
      └─ running_ = false → worker_.join()
```

---

## 5-3. 스레드 안전성 정리

| 상황 | 처리 방법 |
|------|-----------|
| 메인 스레드 DataStore 접근 | 각 메서드 내부에서 lock_guard |
| 백그라운드 스레드 DataStore 접근 | 동일한 mutex로 직렬화 |
| 동일 초 복수 주문 생성 | generateOrderId의 atomic counter로 구분 |
| stop() 호출 시 workerLoop 종료 | atomic<bool> running_ = false + thread join |

---

## 5-4. 프로그램 종료 흐름

```
사용자 "0" 입력
→ MainMenuView::run() 반환
→ ProductionController::stop() 호출
   → running_ = false
   → workerLoop가 다음 sleep(1) 후 루프 종료
   → worker_.join() 완료
→ main() return 0
→ DataStore 소멸자는 정의하지 않음 (모든 변경은 즉시 save되므로 추가 저장 불필요)
```

---

## 5-5. 예외 처리 전략

| 상황 | 처리 |
|------|------|
| `DB/data.json` 파일 없음 | load()에서 ifstream open 실패 시 그냥 반환 (빈 상태) |
| JSON 파싱 오류 | `json::parse(..., false)` 로 discarded 반환 → 빈 상태 유지 |
| 존재하지 않는 시료 ID로 주문 | createOrder가 빈 문자열 반환 → View에서 오류 메시지 출력 |
| 존재하지 않는 주문번호 | Controller가 false 반환 → View에서 "처리 실패" 출력 |
| 생산 완료 시 시료 정보 없음 | onJobComplete에서 early return |

콘솔 프로그램 특성상 예외가 main까지 전파되면 프로그램이 종료된다.  
`DataStore::load/save`의 파일 I/O는 try-catch 없이 운용하며,  
파일 쓰기 실패는 무시한다. (운영 환경에서는 로그 추가 고려)

---

## 5-6. 빌드 및 실행 확인 체크리스트

### 빌드
- [ ] 모든 소스 파일이 `Semi.vcxproj`에 등록됨
- [ ] `Semi/Lib/json.hpp` 존재
- [ ] C++20 표준 설정 (`stdcpp20`)
- [ ] 컴파일 오류 없음

### 실행 (수동 테스트 시나리오)

**시나리오 A — 기본 플로우 (재고 있는 경우)**
```
1. 시료 관리 → 시료 등록: ID=S001, 이름=GaN Wafer, 생산시간=5, 수율=0.9
2. (DataStore에 직접 stock=20 설정하거나, 시나리오 B 선행)
3. 주문 → 주문 접수: 고객=삼성전자, 시료=S001, 수량=5
4. 주문 → 주문 승인/거절: 해당 주문 승인
   → 재고 충분이면 즉시 Confirmed
5. 출고 처리: Confirmed 주문 출고 → Release
6. 모니터링 → 주문량 확인: Release 1건 확인
```

**시나리오 B — 생산 플로우 (재고 없는 경우)**
```
1. 시료 등록: ID=S001, 생산시간=5, 수율=1.0
2. 주문 접수: 수량=3
3. 주문 승인 → 재고 0 → Producing + Job 생성
4. 생산 라인 메뉴: Job 목록 및 남은 시간 확인
5. 5초×3 = 15초 대기
6. 모니터링 → 주문량 확인: Confirmed 1건 확인
7. 출고 처리
```

**시나리오 C — 영속화 확인**
```
1. 시나리오 A 수행 후 프로그램 종료
2. DB/data.json 내용 확인 (텍스트 에디터)
3. 프로그램 재시작 → 모니터링에서 이전 상태 복원 확인
```

---

## 완료 기준

- 빌드 오류/경고 없이 컴파일
- 시나리오 A, B, C 모두 정상 동작
- 프로그램 종료 후 `DB/data.json`에 최신 상태 저장됨
- 재시작 후 이전 상태 완전 복원
