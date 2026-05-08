#include "ProductionLineView.h"
#include "../Controller/ProductionController.h"
#include "../Controller/SampleController.h"
#include <iostream>
#include <iomanip>
#include <ctime>

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

void ProductionLineView::show() const {
    auto now         = std::time(nullptr);
    auto activeJobs  = ProductionController::instance().getActiveJobs();
    auto pendingJobs = ProductionController::instance().getPendingJobs();
    SampleController sampleCtrl;

    std::cout << "\n--- 생산 라인 ---\n";

    std::cout << "[현재 생산 중]\n";
    if (activeJobs.empty()) {
        std::cout << "  (없음)\n";
    } else {
        std::cout << std::left
                  << std::setw(25) << "주문번호"
                  << std::setw(20) << "시료이름"
                  << std::setw(8)  << "주문량"
                  << std::setw(8)  << "재고"
                  << std::setw(12) << "필요생산량"
                  << std::setw(12) << "완료 예정" << "\n";
        std::cout << std::string(85, '-') << "\n";
        for (const auto& job : activeJobs) {
            auto sAll = sampleCtrl.getAllSamples();
            std::string sampleName = job.sampleId;
            int stock = 0;
            for (const auto& s : sAll) {
                if (s.id == job.sampleId) { sampleName = s.name; stock = s.stock; break; }
            }
            std::time_t finishTime = job.startTime + job.durationSec;
            std::cout << std::left
                      << std::setw(25) << job.orderId
                      << std::setw(20) << sampleName
                      << std::setw(8)  << job.quantity
                      << std::setw(8)  << stock
                      << std::setw(12) << job.shortfall
                      << std::setw(12) << formatTime(finishTime) << "\n";
        }
    }

    std::cout << "\n[생산 대기 큐] (최대 3개)\n";
    if (pendingJobs.empty()) {
        std::cout << "  (없음)\n";
    } else {
        std::cout << std::left
                  << std::setw(6)  << "순서"
                  << std::setw(25) << "주문번호"
                  << std::setw(20) << "시료이름"
                  << std::setw(8)  << "주문량"
                  << std::setw(12) << "필요생산량"
                  << std::setw(12) << "완료 예정" << "\n";
        std::cout << std::string(83, '-') << "\n";

        std::time_t nextStart = activeJobs.empty()
            ? now
            : activeJobs[0].startTime + activeJobs[0].durationSec;

        int count = 0;
        for (const auto& job : pendingJobs) {
            if (count >= 3) break;
            auto sAll = sampleCtrl.getAllSamples();
            std::string sampleName = job.sampleId;
            for (const auto& s : sAll) {
                if (s.id == job.sampleId) { sampleName = s.name; break; }
            }
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
}
