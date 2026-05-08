#pragma once
#include "../Model/Order.h"
#include <vector>
#include <string>

class OrderController {
public:
    std::string        createOrder(const std::string& customer,
                                   const std::string& sampleId,
                                   int quantity);
    bool               approveOrder(const std::string& orderId);
    bool               rejectOrder(const std::string& orderId);
    std::vector<Order> getReservedOrders() const;

private:
    std::string        generateOrderId() const;
};
