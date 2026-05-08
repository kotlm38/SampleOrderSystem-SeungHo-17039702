#pragma once
#include "../Model/Order.h"
#include <vector>
#include <string>

class ShippingController {
public:
    std::vector<Order> getConfirmedOrders() const;
    bool               processShipping(const std::string& orderId);
};
