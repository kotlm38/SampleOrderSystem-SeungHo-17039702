#pragma once
#include "../Model/ProductionJob.h"
#include <vector>
#include <thread>
#include <atomic>

// 백그라운드 스레드로 생산 큐를 감시하며 완료된 작업을 처리하는 싱글톤
class ProductionController {
public:
    static ProductionController& instance();

    void start();  // 백그라운드 스레드 시작
    void stop();   // 백그라운드 스레드 종료 (프로그램 종료 시 호출)

    // 현재 생산 중인 작업 목록 조회 (View용)
    std::vector<ProductionJob> getActiveJobs() const;

private:
    ProductionController() = default;
    ProductionController(const ProductionController&) = delete;
    ProductionController& operator=(const ProductionController&) = delete;

    void workerLoop();
    void onJobComplete(const ProductionJob& job);  // 생산 완료 처리

    std::thread       worker_;
    std::atomic<bool> running_{ false };
};
