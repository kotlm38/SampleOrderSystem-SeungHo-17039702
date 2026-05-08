#pragma once
#include <string>
#include <ctime>

struct ProductionJob {
    std::string  jobId;
    std::string  orderId;
    std::string  sampleId;
    int          quantity;
    std::time_t  startTime;
    int          durationSec;  // avgProductionTimeSec * quantity
};
