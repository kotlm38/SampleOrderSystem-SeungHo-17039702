#pragma once
#include <ctime>
#include <string>
#include <map>
#include "../Model/Sample.h"
#include "../Model/ProductionJob.h"
#include <vector>

class ProductionLineView {
public:
    void show() const;
private:
    static std::string formatTime(std::time_t t);
    void printActiveJobsTable(const std::vector<ProductionJob>& jobs,
                              const std::map<std::string, Sample>& sampleMap) const;
    void printPendingJobsTable(const std::vector<ProductionJob>& activeJobs,
                               const std::vector<ProductionJob>& pendingJobs,
                               const std::map<std::string, Sample>& sampleMap) const;
};
