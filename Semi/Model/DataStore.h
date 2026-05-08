#pragma once
#include "Sample.h"
#include "Order.h"
#include "ProductionJob.h"
#include <vector>
#include <mutex>
#include <string>
#include <optional>

class DataStore {
public:
    static DataStore& instance();

    void load();
    void save();

    std::vector<Sample>        getSamples() const;
    std::optional<Sample>      findSample(const std::string& id) const;
    void                       addSample(const Sample& s);
    void                       updateSample(const Sample& s);

    std::vector<Order>         getOrders() const;
    std::optional<Order>       findOrder(const std::string& orderId) const;
    void                       addOrder(const Order& o);
    void                       updateOrder(const Order& o);
    void                       removeOrder(const std::string& orderId);

    std::vector<ProductionJob> getProductionJobs() const;
    void                       addProductionJob(const ProductionJob& job);
    void                       updateProductionJob(const ProductionJob& job);
    void                       removeProductionJob(const std::string& jobId);

private:
    DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    void saveUnlocked() const;
    static std::string dbFilePath();

    mutable std::mutex          mutex_;
    std::vector<Sample>         samples_;
    std::vector<Order>          orders_;
    std::vector<ProductionJob>  jobs_;
};
