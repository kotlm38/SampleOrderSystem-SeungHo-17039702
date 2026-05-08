#pragma once

class OrderView {
public:
    void show();  // 주문 서브메뉴 진입

private:
    void printMenu() const;
    void doCreateOrder();
    void doReviewOrders();  // 승인 / 거절
};
