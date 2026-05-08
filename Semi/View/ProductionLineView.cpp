#include "ProductionLineView.h"
#include "../Controller/ProductionController.h"
#include "../Controller/SampleController.h"
#include <iostream>
#include <iomanip>
#include <ctime>

static constexpr int MAX_PENDING_DISPLAY  = 3;
static constexpr int ACTIVE_TABLE_WIDTH   = 85;
static constexpr int PENDING_TABLE_WIDTH  = 83;

std::string ProductionLineView::formatTime(std::time_t t) {
    struct tm tmBuf;
#ifdef _WIN32
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tmBuf);
    return buf;
}

void ProductionLineView::printActiveJobsTable(
        const std::vector<ProductionJob>& jobs,
        const std::map<std::string, Sample>& sampleMap) const {
    std::cout << std::left
              << std::setw(25) << "주문번호"
              << std::setw(20) << "시료이름"
              << std::setw(8)  << "주문량"
              << std::setw(8)  << "재고"
              << std::setw(12) << "필요생산량"
              << std::setw(12) << "완료 예정" << "\n";
    std::cout << std::string(ACTIVE_TABLE_WIDTH, '-') << "\n";
    for (const auto& job : jobs) {
        auto it = sampleMap.find(job.sampleId);
        std::string sampleName = (it != sampleMap.end()) ? it->second.name : job.sampleId;
        int stock              = (it != sampleMap.end()) ? it->second.stock : 0;
        std::cout << std::left
                  << std::setw(25) << job.orderId
                  << std::setw(20) << sampleName
                  << std::setw(8)  << job.quantity
                  << std::setw(8)  << stock
                  << std::setw(12) << job.shortfall
                  << std::setw(12) << formatTime(job.startTime + job.durationSec) << "\n";
    }
}

void ProductionLineView::printPendingJobsTable(
        const std::vector<ProductionJob>& activeJobs,
        const std::vector<ProductionJob>& pendingJobs,
        const std::map<std::string, Sample>& sampleMap) const {
    std::cout << std::left
              << std::setw(6)  << "순서"
              << std::setw(25) << "주문번호"
              << std::setw(20) << "시료이름"
              << std::setw(8)  << "주문량"
              << std::setw(12) << "필요생산량"
              << std::setw(12) << "완료 예정" << "\n";
    std::cout << std::string(PENDING_TABLE_WIDTH, '-') << "\n";

    std::time_t nextStart = activeJobs.empty()
        ? std::time(nullptr)
        : activeJobs[0].startTime + activeJobs[0].durationSec;

    int count = 0;
    for (const auto& job : pendingJobs) {
        if (count >= MAX_PENDING_DISPLAY) break;
        auto it = sampleMap.find(job.sampleId);
        std::string sampleName = (it != sampleMap.end()) ? it->second.name : job.sampleId;
        std::time_t estimatedFinish = nextStart + job.durationSec;
        nextStart = estimatedFinish;
        std::cout << std::left
                  << std::setw(6)  << (count + 1)
                  << std::setw(25) << job.orderId
                  << std::setw(20) << sampleName
                  << std::setw(8)  << job.quantity
                  << std::setw(12) << job.shortfall
                  << std::setw(12) << formatTime(estimatedFinish) << "\n";
        ++count;
    }
}

void ProductionLineView::show() const {
    auto activeJobs  = ProductionController::instance().getActiveJobs();
    auto pendingJobs = ProductionController::instance().getPendingJobs();

    std::map<std::string, Sample> sampleMap;
    for (const auto& s : SampleController{}.getAllSamples())
        sampleMap[s.id] = s;

    std::cout << "\n--- 생산 라인 ---\n";

    std::cout << "[현재 생산 중]\n";
    if (activeJobs.empty())
        std::cout << "  (없음)\n";
    else
        printActiveJobsTable(activeJobs, sampleMap);

    std::cout << "\n[생산 대기 큐] (최대 " << MAX_PENDING_DISPLAY << "개)\n";
    if (pendingJobs.empty())
        std::cout << "  (없음)\n";
    else
        printPendingJobsTable(activeJobs, pendingJobs, sampleMap);
}
