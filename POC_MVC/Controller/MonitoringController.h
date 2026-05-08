#pragma once
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include <vector>
#include <string>

enum class StockStatus {
    Sufficient,  // 여유: 주문 대비 재고 충분
    Shortage,    // 부족: 재고가 있으나 부족
    Depleted     // 고갈: 재고 0
};

struct InventoryInfo {
    Sample      sample;
    StockStatus stockStatus;
};

class MonitoringController {
public:
    // 상태별 주문 목록
    std::vector<Order>         getOrdersByStatus(OrderStatus status) const;

    // 시료별 재고 현황 (주문 대비 상태 포함)
    std::vector<InventoryInfo> getInventoryStatus() const;
};
