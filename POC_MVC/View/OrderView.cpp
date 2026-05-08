#include "OrderView.h"
#include "../Controller/OrderController.h"
#include "../Model/Order.h"
#include <iostream>
#include <iomanip>

void OrderView::show() {
    int choice = -1;
    while (choice != 0) {
        printMenu();
        std::cin >> choice;
        std::cin.ignore();
        switch (choice) {
            case 1: doCreateOrder();  break;
            case 2: doReviewOrders(); break;
            case 0:                   break;
            default: std::cout << "잘못된 입력입니다.\n";
        }
    }
}

void OrderView::printMenu() const {
    std::cout << "\n--- 주문 관리 ---\n";
    std::cout << "1. 주문 접수 (주문담당자)\n";
    std::cout << "2. 주문 승인 / 거절 (생산담당자)\n";
    std::cout << "0. 뒤로\n";
    std::cout << "선택: ";
}

void OrderView::doCreateOrder() {
    std::string sampleId;
    int qty;
    std::cout << "시료 ID: ";    std::getline(std::cin, sampleId);
    std::cout << "수량: ";       std::cin >> qty;
    std::cin.ignore();

    OrderController ctrl;
    std::string orderId = ctrl.createOrder(sampleId, qty);
    if (orderId.empty())
        std::cout << "존재하지 않는 시료 ID입니다.\n";
    else
        std::cout << "주문 접수 완료. 주문번호: " << orderId << "\n";
}

void OrderView::doReviewOrders() {
    OrderController ctrl;
    auto reserved = ctrl.getReservedOrders();
    if (reserved.empty()) { std::cout << "승인 대기 중인 주문이 없습니다.\n"; return; }

    std::cout << "\n[Reserved 주문 목록]\n";
    std::cout << std::left << std::setw(25) << "주문번호"
              << std::setw(15) << "시료ID"
              << std::setw(8)  << "수량" << "\n";
    std::cout << std::string(48, '-') << "\n";
    for (const auto& o : reserved) {
        std::cout << std::left << std::setw(25) << o.orderId
                  << std::setw(15) << o.sampleId
                  << std::setw(8)  << o.quantity << "\n";
    }

    std::cout << "\n처리할 주문번호 (취소: 0): ";
    std::string orderId;
    std::getline(std::cin, orderId);
    if (orderId == "0") return;

    std::cout << "1. 승인  2. 거절: ";
    int action;
    std::cin >> action;
    std::cin.ignore();

    if (action == 1) {
        ctrl.approveOrder(orderId)
            ? std::cout << "승인 완료.\n"
            : std::cout << "처리 실패.\n";
    } else if (action == 2) {
        ctrl.rejectOrder(orderId)
            ? std::cout << "거절 완료.\n"
            : std::cout << "처리 실패.\n";
    }
}
