#pragma once
#include "../Model/Order.h"
#include "../Model/Sample.h"
#include <vector>

enum class StockStatus {
    Sufficient,
    Shortage,
    Depleted
};

struct InventoryInfo {
    Sample      sample;
    StockStatus stockStatus;
};

class MonitoringController {
public:
    std::vector<Order>         getOrdersByStatus(OrderStatus status) const;
    std::vector<InventoryInfo> getInventoryStatus() const;
};
