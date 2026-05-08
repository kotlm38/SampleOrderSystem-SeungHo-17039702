#include "SampleController.h"
#include "../Model/DataStore.h"
#include <algorithm>

bool SampleController::registerSample(const std::string& id,
                                      const std::string& name,
                                      int avgProductionTimeSec,
                                      float yield) {
    if (DataStore::instance().findSample(id).has_value()) return false;

    Sample s;
    s.id                  = id;
    s.name                = name;
    s.avgProductionTimeSec = avgProductionTimeSec;
    s.yield               = yield;
    s.stock               = 0;

    DataStore::instance().addSample(s);
    return true;
}

std::vector<Sample> SampleController::getAllSamples() const {
    return DataStore::instance().getSamples();
}

std::vector<Sample> SampleController::searchByName(const std::string& keyword) const {
    auto all = DataStore::instance().getSamples();
    std::vector<Sample> result;
    for (const auto& s : all) {
        if (s.name.find(keyword) != std::string::npos)
            result.push_back(s);
    }
    return result;
}
