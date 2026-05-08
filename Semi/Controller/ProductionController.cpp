#include "ProductionController.h"
#include "../Model/DataStore.h"
#include <chrono>
#include <ctime>
#include <cmath>

ProductionController& ProductionController::instance() {
    static ProductionController inst;
    return inst;
}

void ProductionController::start() {
    running_ = true;
    worker_  = std::thread(&ProductionController::workerLoop, this);
}

void ProductionController::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

void ProductionController::workerLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = std::time(nullptr);

        auto jobs = DataStore::instance().getProductionJobs();
        for (const auto& job : jobs) {
            if (job.startTime > 0 && now >= job.startTime + job.durationSec) {
                onJobComplete(job);
            }
        }

        jobs = DataStore::instance().getProductionJobs();
        bool hasActive = false;
        for (const auto& j : jobs) {
            if (j.startTime > 0) { hasActive = true; break; }
        }
        if (!hasActive) {
            for (const auto& j : jobs) {
                if (j.startTime == 0) {
                    ProductionJob started = j;
                    started.startTime = now;
                    DataStore::instance().updateProductionJob(started);
                    break;
                }
            }
        }
    }
}

void ProductionController::onJobComplete(const ProductionJob& job) {
    auto sampleOpt = DataStore::instance().findSample(job.sampleId);
    if (!sampleOpt.has_value()) return;

    Sample sample = *sampleOpt;
    int producedQty = static_cast<int>(std::ceil(job.quantity * sample.yield));
    sample.stock += producedQty - job.shortfall;
    if (sample.stock < 0) sample.stock = 0;
    DataStore::instance().updateSample(sample);

    auto orderOpt = DataStore::instance().findOrder(job.orderId);
    if (orderOpt.has_value()) {
        Order order    = *orderOpt;
        order.status   = OrderStatus::Confirmed;
        order.updatedAt = std::time(nullptr);
        DataStore::instance().updateOrder(order);
    }

    DataStore::instance().removeProductionJob(job.jobId);
}

std::vector<ProductionJob> ProductionController::getActiveJobs() const {
    auto all = DataStore::instance().getProductionJobs();
    std::vector<ProductionJob> result;
    for (const auto& j : all) {
        if (j.startTime > 0) result.push_back(j);
    }
    return result;
}

std::vector<ProductionJob> ProductionController::getPendingJobs() const {
    auto all = DataStore::instance().getProductionJobs();
    std::vector<ProductionJob> result;
    for (const auto& j : all) {
        if (j.startTime == 0) result.push_back(j);
    }
    return result;
}
