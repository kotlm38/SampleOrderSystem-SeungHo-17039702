#pragma once
#include "../Model/Order.h"
#include <vector>
#include <string>

class ShippingController {
public:
    // Confirmed 상태 주문 목록 조회
    std::vector<Order> getConfirmedOrders() const;

    // 출고 처리: Confirmed → Release
    bool               processShipping(const std::string& orderId);
};
