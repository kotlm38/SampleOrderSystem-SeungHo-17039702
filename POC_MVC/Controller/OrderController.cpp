#include "OrderController.h"
#include "../Model/DataStore.h"
#include "../Model/ProductionJob.h"
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>

std::string OrderController::generateOrderId() const {
    // TODO: UUID 또는 타임스탬프 기반 고유 ID 생성
    return "ORD-" + std::to_string(std::time(nullptr));
}

std::string OrderController::createOrder(const std::string& sampleId, int quantity) {
    auto sample = DataStore::instance().findSample(sampleId);
    if (!sample.has_value()) return "";  // 존재하지 않는 시료

    Order order;
    order.orderId   = generateOrderId();
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
        // 재고 충분 → Confirmed
        sample.stock -= order.quantity;
        order.status  = OrderStatus::Confirmed;
        order.updatedAt = std::time(nullptr);
        DataStore::instance().updateSample(sample);
        DataStore::instance().updateOrder(order);
    } else {
        // 재고 부족 → Producing + 생산 큐 등록
        order.status  = OrderStatus::Producing;
        order.updatedAt = std::time(nullptr);
        DataStore::instance().updateOrder(order);

        ProductionJob job;
        job.jobId       = "JOB-" + orderId;
        job.orderId     = orderId;
        job.sampleId    = sample.id;
        job.quantity    = order.quantity;
        job.startTime   = std::time(nullptr);
        job.durationSec = sample.avgProductionTimeSec * order.quantity;
        DataStore::instance().addProductionJob(job);
    }
    return true;
}

bool OrderController::rejectOrder(const std::string& orderId) {
    auto orderOpt = DataStore::instance().findOrder(orderId);
    if (!orderOpt.has_value()) return false;

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
