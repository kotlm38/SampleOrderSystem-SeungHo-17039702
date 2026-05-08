#pragma once
#include "../Model/ProductionJob.h"
#include <vector>
#include <thread>
#include <atomic>

class ProductionController {
public:
    static ProductionController& instance();

    void start();
    void stop();

    std::vector<ProductionJob> getActiveJobs()  const;
    std::vector<ProductionJob> getPendingJobs() const;

private:
    ProductionController() = default;
    ProductionController(const ProductionController&) = delete;
    ProductionController& operator=(const ProductionController&) = delete;

    void workerLoop();
    void onJobComplete(const ProductionJob& job);

    std::thread        worker_;
    std::atomic<bool>  running_{ false };
};
