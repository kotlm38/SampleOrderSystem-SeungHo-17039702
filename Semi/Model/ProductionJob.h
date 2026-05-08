#pragma once
#include <string>
#include <ctime>

struct ProductionJob {
    std::string  jobId;
    std::string  orderId;
    std::string  sampleId;
    int          quantity;
    int          shortfall;
    std::time_t  startTime;
    int          durationSec;
};
