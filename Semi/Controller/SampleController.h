#pragma once
#include "../Model/Sample.h"
#include <vector>
#include <string>

enum class SampleSearchField {
    Id,
    Name,
    AvgProductionTimeSec,
    Yield
};

class SampleController {
public:
    bool                registerSample(const std::string& id,
                                       const std::string& name,
                                       int avgProductionTimeSec,
                                       float yield);
    std::vector<Sample> getAllSamples() const;
    std::vector<Sample> search(SampleSearchField field,
                               const std::string& keyword) const;
};
