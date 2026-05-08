#pragma once
#include "../Model/Order.h"
#include <vector>

class ShippingView {
public:
    void show();
private:
    void printConfirmedOrders(const std::vector<Order>& confirmed) const;
};
