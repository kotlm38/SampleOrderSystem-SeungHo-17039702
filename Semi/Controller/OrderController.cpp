#include "OrderController.h"
#include "../Model/DataStore.h"
#include "../Model/ProductionJob.h"
#include <ctime>
#include <atomic>
#include <cmath>

std::string OrderController::generateOrderId() const {
    static std::atomic<int> s_counter{ 0 };
    return "ORD-" + std::to_string(std::time(nullptr))
                  + "-" + std::to_string(s_counter++);
}

std::string OrderController::createOrder(const std::string& customer,
                                          const std::string& sampleId,
                                          int quantity) {
    if (!DataStore::instance().findSample(sampleId).has_value()) return "";

    Order order;
    order.orderId   = generateOrderId();
    order.customer  = customer;
    order.sampleId  = sampleId;
    order.quantity  = quantity;
    order.status    = OrderStatus::Reserved;
    order.createdAt = std::time(nullptr);
    order.updatedAt = order.createdAt;

    DataStore::instance().addOrder(order);
    return order.orderId;
}

bool OrderController::approveOrder(const std::string& orderId) {
    auto orderOpt = DataStore::instance().findOrder(orderId);
    if (!orderOpt.has_value()) return false;

    auto sampleOpt = DataStore::instance().findSample(orderOpt->sampleId);
    if (!sampleOpt.has_value()) return false;

    Order  order  = *orderOpt;
    Sample sample = *sampleOpt;

    if (sample.stock >= order.quantity) {
        sample.stock -= order.quantity;
        order.status  = OrderStatus::Confirmed;
        order.updatedAt = std::time(nullptr);
        DataStore::instance().updateSample(sample);
        DataStore::instance().updateOrder(order);
    } else {
        int shortfall  = order.quantity - sample.stock;
        int produceQty = static_cast<int>(std::ceil(
            static_cast<float>(shortfall) / (sample.yield * 0.9f)));

        order.status    = OrderStatus::Producing;
        order.updatedAt = std::time(nullptr);
        DataStore::instance().updateOrder(order);

        sample.stock = 0;
        DataStore::instance().updateSample(sample);

        ProductionJob job;
        job.jobId       = "JOB-" + orderId;
        job.orderId     = orderId;
        job.sampleId    = sample.id;
        job.quantity    = produceQty;
        job.shortfall   = shortfall;
        job.startTime   = 0;
        job.durationSec = sample.avgProductionTimeSec * produceQty;
        DataStore::instance().addProductionJob(job);
    }
    return true;
}

bool OrderController::rejectOrder(const std::string& orderId) {
    if (!DataStore::instance().findOrder(orderId).has_value()) return false;
    DataStore::instance().removeOrder(orderId);
    return true;
}

std::vector<Order> OrderController::getReservedOrders() const {
    auto all = DataStore::instance().getOrders();
    std::vector<Order> result;
    for (const auto& o : all) {
        if (o.status == OrderStatus::Reserved)
            result.push_back(o);
    }
    return result;
}
