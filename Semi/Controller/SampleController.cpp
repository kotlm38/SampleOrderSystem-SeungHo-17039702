#include "SampleController.h"
#include "../Model/DataStore.h"
#include <algorithm>

bool SampleController::registerSample(const std::string& id,
                                      const std::string& name,
                                      int avgProductionTimeSec,
                                      float yield) {
    if (DataStore::instance().findSample(id).has_value()) return false;
    Sample s;
    s.id                   = id;
    s.name                 = name;
    s.avgProductionTimeSec = avgProductionTimeSec;
    s.yield                = yield;
    s.stock                = 0;
    DataStore::instance().addSample(s);
    return true;
}

std::vector<Sample> SampleController::getAllSamples() const {
    return DataStore::instance().getSamples();
}

std::vector<Sample> SampleController::search(SampleSearchField field,
                                              const std::string& keyword) const {
    auto all = DataStore::instance().getSamples();
    std::vector<Sample> result;
    for (const auto& s : all) {
        std::string attr;
        switch (field) {
            case SampleSearchField::Id:                   attr = s.id; break;
            case SampleSearchField::Name:                 attr = s.name; break;
            case SampleSearchField::AvgProductionTimeSec: attr = std::to_string(s.avgProductionTimeSec); break;
            case SampleSearchField::Yield:                attr = std::to_string(s.yield); break;
        }
        if (attr.find(keyword) != std::string::npos)
            result.push_back(s);
    }
    return result;
}
