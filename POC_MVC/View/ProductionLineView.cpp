#include "ProductionLineView.h"
#include "../Controller/ProductionController.h"
#include <iostream>
#include <iomanip>
#include <ctime>

void ProductionLineView::show() const {
    auto jobs = ProductionController::instance().getActiveJobs();
    auto now  = std::time(nullptr);

    std::cout << "\n--- 생산 라인 ---\n";
    if (jobs.empty()) { std::cout << "현재 생산 중인 작업이 없습니다.\n"; return; }

    std::cout << std::left << std::setw(18) << "Job ID"
              << std::setw(25) << "주문번호"
              << std::setw(12) << "시료ID"
              << std::setw(8)  << "수량"
              << std::setw(14) << "남은시간(초)" << "\n";
    std::cout << std::string(77, '-') << "\n";

    for (const auto& job : jobs) {
        int elapsed   = static_cast<int>(now - job.startTime);
        int remaining = std::max(0, job.durationSec - elapsed);
        std::cout << std::left << std::setw(18) << job.jobId
                  << std::setw(25) << job.orderId
                  << std::setw(12) << job.sampleId
                  << std::setw(8)  << job.quantity
                  << std::setw(14) << remaining << "\n";
    }
}
