#include "Model/DataStore.h"
#include "Controller/ProductionController.h"
#include "View/MainMenuView.h"

int main() {
    // 이전 상태 복원
    DataStore::instance().load();

    // 백그라운드 생산 스레드 시작
    ProductionController::instance().start();

    // 메인 메뉴 루프 실행
    MainMenuView{}.run();

    // 종료 시 생산 스레드 정리
    ProductionController::instance().stop();

    return 0;
}
