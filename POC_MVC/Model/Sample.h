#pragma once
#include <string>

struct Sample {
    std::string id;
    std::string name;
    int         avgProductionTimeSec;  // 평균 생산 시간 (초)
    float       yield;                 // 수율 (0.0 ~ 1.0)
    int         stock;                 // 현재 재고
};
