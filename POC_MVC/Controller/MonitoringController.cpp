#include "MonitoringController.h"
#include "../Model/DataStore.h"
#include <algorithm>
#include <numeric>

std::vector<Order> MonitoringController::getOrdersByStatus(OrderStatus status) const {
    auto all = DataStore::instance().getOrders();
    std::vector<Order> result;
    for (const auto& o : all) {
        if (o.status == status) result.push_back(o);
    }
    return result;
}

std::vector<InventoryInfo> MonitoringController::getInventoryStatus() const {
    auto samples = DataStore::instance().getSamples();
    auto orders  = DataStore::instance().getOrders();

    std::vector<InventoryInfo> result;
    for (const auto& s : samples) {
        // 현재 활성 주문(Reserved + Producing)의 총 요구 수량 합산
        int demanded = 0;
        for (const auto& o : orders) {
            if (o.sampleId == s.id &&
                (o.status == OrderStatus::Reserved || o.status == OrderStatus::Producing)) {
                demanded += o.quantity;
            }
        }

        InventoryInfo info;
        info.sample = s;
        if (s.stock == 0)             info.stockStatus = StockStatus::Depleted;
        else if (s.stock < demanded)  info.stockStatus = StockStatus::Shortage;
        else                          info.stockStatus = StockStatus::Sufficient;

        result.push_back(info);
    }
    return result;
}
