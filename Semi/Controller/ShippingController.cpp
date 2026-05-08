#include "ShippingController.h"
#include "../Model/DataStore.h"
#include <ctime>

std::vector<Order> ShippingController::getConfirmedOrders() const {
    auto all = DataStore::instance().getOrders();
    std::vector<Order> result;
    for (const auto& o : all) {
        if (o.status == OrderStatus::Confirmed) result.push_back(o);
    }
    return result;
}

bool ShippingController::processShipping(const std::string& orderId) {
    auto orderOpt = DataStore::instance().findOrder(orderId);
    if (!orderOpt.has_value()) return false;
    if (orderOpt->status != OrderStatus::Confirmed) return false;

    Order order    = *orderOpt;
    order.status   = OrderStatus::Release;
    order.updatedAt = std::time(nullptr);
    DataStore::instance().updateOrder(order);
    return true;
}
