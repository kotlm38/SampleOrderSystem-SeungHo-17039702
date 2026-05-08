#include "MainMenuView.h"
#include "SampleView.h"
#include "OrderView.h"
#include "MonitoringView.h"
#include "ShippingView.h"
#include "ProductionLineView.h"
#include <iostream>

void MainMenuView::run() {
    int choice = -1;
    while (choice != 0) {
        printMenu();
        std::cin >> choice;
        std::cin.ignore();
        dispatch(choice);
    }
    std::cout << "프로그램을 종료합니다.\n";
}

void MainMenuView::printMenu() const {
    std::cout << "\n========= 반도체 시료 관리 시스템 =========\n";
    std::cout << "1. 시료 관리\n";
    std::cout << "2. 주문 (접수 / 승인 / 거절)\n";
    std::cout << "3. 모니터링\n";
    std::cout << "4. 출고 처리\n";
    std::cout << "5. 생산 라인\n";
    std::cout << "0. 종료\n";
    std::cout << "선택: ";
}

void MainMenuView::dispatch(int choice) {
    switch (choice) {
        case 1: SampleView{}.show();          break;
        case 2: OrderView{}.show();           break;
        case 3: MonitoringView{}.show();      break;
        case 4: ShippingView{}.show();        break;
        case 5: ProductionLineView{}.show();  break;
        case 0:                               break;
        default: std::cout << "잘못된 입력입니다.\n";
    }
}
