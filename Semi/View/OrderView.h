#pragma once

class OrderView {
public:
    void show();
private:
    void printMenu() const;
    void doCreateOrder();
    void doReviewOrders();
};
