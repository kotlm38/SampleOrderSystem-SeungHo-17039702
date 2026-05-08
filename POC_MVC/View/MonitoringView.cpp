#include "MonitoringView.h"
#include "../Controller/MonitoringController.h"
#include "../Model/Order.h"
#include <iostream>
#include <iomanip>

static const char* stockStatusLabel(StockStatus s) {
    switch (s) {
        case StockStatus::Sufficient: return "여유";
        case StockStatus::Shortage:   return "부족";
        case StockStatus::Depleted:   return "고갈";
        default:                      return "-";
    }
}

void MonitoringView::show() {
    int choice = -1;
    while (choice != 0) {
        printMenu();
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 1: doOrderCount(); break;
            case 2: doInventory();  break;
            case 0:                 break;
            default: std::cout << "잘못된 입력입니다.\n";
        }
    }
}

void MonitoringView::printMenu() const {
    std::cout << "\n--- 모니터링 ---\n";
    std::cout << "1. 주문량 확인\n";
    std::cout << "2. 재고량 확인\n";
    std::cout << "0. 뒤로\n";
    std::cout << "선택: ";
}

void MonitoringView::doOrderCount() const {
    MonitoringController ctrl;
    const OrderStatus statuses[] = {
        OrderStatus::Reserved, OrderStatus::Producing,
        OrderStatus::Confirmed, OrderStatus::Release
    };
    std::cout << "\n[상태별 주문 현황]\n";
    for (auto st : statuses) {
        auto orders = ctrl.getOrdersByStatus(st);
        std::cout << std::left << std::setw(12) << orderStatusToString(st)
                  << ": " << orders.size() << "건\n";
        for (const auto& o : orders) {
            std::cout << "  - " << o.orderId
                      << "  시료: " << o.sampleId
                      << "  수량: " << o.quantity << "\n";
        }
    }
}

void MonitoringView::doInventory() const {
    MonitoringController ctrl;
    auto infos = ctrl.getInventoryStatus();
    if (infos.empty()) { std::cout << "등록된 시료가 없습니다.\n"; return; }

    std::cout << "\n[시료별 재고 현황]\n";
    std::cout << std::left << std::setw(12) << "ID"
              << std::setw(20) << "이름"
              << std::setw(8)  << "재고"
              << std::setw(8)  << "상태" << "\n";
    std::cout << std::string(48, '-') << "\n";
    for (const auto& info : infos) {
        std::cout << std::left << std::setw(12) << info.sample.id
                  << std::setw(20) << info.sample.name
                  << std::setw(8)  << info.sample.stock
                  << std::setw(8)  << stockStatusLabel(info.stockStatus) << "\n";
    }
}
