#include "ProductionController.h"
#include "../Model/DataStore.h"
#include <chrono>
#include <ctime>

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

        auto jobs    = DataStore::instance().getProductionJobs();
        auto now     = std::time(nullptr);

        for (const auto& job : jobs) {
            if (now >= job.startTime + job.durationSec) {
                onJobComplete(job);
            }
        }
    }
}

void ProductionController::onJobComplete(const ProductionJob& job) {
    // 수율 적용 후 재고 증가
    auto sampleOpt = DataStore::instance().findSample(job.sampleId);
    if (!sampleOpt.has_value()) return;

    Sample sample  = *sampleOpt;
    sample.stock  += static_cast<int>(job.quantity * sample.yield);
    DataStore::instance().updateSample(sample);

    // 주문 상태 Confirmed 로 전환
    auto orderOpt = DataStore::instance().findOrder(job.orderId);
    if (orderOpt.has_value()) {
        Order order    = *orderOpt;
        order.status   = OrderStatus::Confirmed;
        order.updatedAt = std::time(nullptr);

        // 생산된 재고에서 차감
        if (sample.stock >= order.quantity) {
            sample.stock -= order.quantity;
            DataStore::instance().updateSample(sample);
        }
        DataStore::instance().updateOrder(order);
    }

    DataStore::instance().removeProductionJob(job.jobId);
}

std::vector<ProductionJob> ProductionController::getActiveJobs() const {
    return DataStore::instance().getProductionJobs();
}
