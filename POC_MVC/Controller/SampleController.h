#pragma once
#include "../Model/Sample.h"
#include <vector>
#include <string>

class SampleController {
public:
    // 새 시료 등록. 중복 ID이면 false 반환
    bool                 registerSample(const std::string& id,
                                        const std::string& name,
                                        int avgProductionTimeSec,
                                        float yield);

    // 전체 시료 목록 반환
    std::vector<Sample>  getAllSamples() const;

    // 이름으로 부분 검색
    std::vector<Sample>  searchByName(const std::string& keyword) const;
};
