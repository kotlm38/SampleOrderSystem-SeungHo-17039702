#include "ShippingView.h"
#include "../Controller/ShippingController.h"
#include <iostream>
#include <iomanip>

void ShippingView::show() {
    printConfirmedOrders();

    ShippingController ctrl;
    auto confirmed = ctrl.getConfirmedOrders();
    if (confirmed.empty()) return;

    std::cout << "\n출고할 주문번호 (취소: 0): ";
    std::string orderId;
    std::getline(std::cin, orderId);
    if (orderId == "0") return;

    ctrl.processShipping(orderId)
        ? std::cout << "출고 완료.\n"
        : std::cout << "처리 실패 (Confirmed 상태가 아니거나 존재하지 않는 주문).\n";
}

void ShippingView::printConfirmedOrders() const {
    ShippingController ctrl;
    auto confirmed = ctrl.getConfirmedOrders();

    std::cout << "\n--- 출고 처리 ---\n";
    if (confirmed.empty()) { std::cout << "출고 가능한 주문이 없습니다.\n"; return; }

    std::cout << std::left << std::setw(25) << "주문번호"
              << std::setw(16) << "고객명"
              << std::setw(12) << "시료ID"
              << std::setw(8)  << "수량" << "\n";
    std::cout << std::string(61, '-') << "\n";
    for (const auto& o : confirmed) {
        std::cout << std::left << std::setw(25) << o.orderId
                  << std::setw(16) << o.customer
                  << std::setw(12) << o.sampleId
                  << std::setw(8)  << o.quantity << "\n";
    }
}
