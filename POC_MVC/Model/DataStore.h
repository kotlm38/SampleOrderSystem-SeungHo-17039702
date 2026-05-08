#pragma once
#include "Sample.h"
#include "Order.h"
#include "ProductionJob.h"
#include <vector>
#include <mutex>
#include <string>
#include <optional>

// 모든 데이터를 중앙 관리하고 JSON 파일로 영속화하는 싱글톤
class DataStore {
public:
    static DataStore& instance();

    void load();   // data.json → 메모리
    void save();   // 메모리 → data.json (상태 변경 시마다 호출)

    // --- Sample ---
    std::vector<Sample>          getSamples() const;
    std::optional<Sample>        findSample(const std::string& id) const;
    void                         addSample(const Sample& sample);
    void                         updateSample(const Sample& sample);  // id로 교체

    // --- Order ---
    std::vector<Order>           getOrders() const;
    std::optional<Order>         findOrder(const std::string& orderId) const;
    void                         addOrder(const Order& order);
    void                         updateOrder(const Order& order);     // orderId로 교체
    void                         removeOrder(const std::string& orderId);

    // --- ProductionJob ---
    std::vector<ProductionJob>   getProductionJobs() const;
    void                         addProductionJob(const ProductionJob& job);
    void                         removeProductionJob(const std::string& jobId);

private:
    DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    mutable std::mutex        mutex_;
    std::vector<Sample>       samples_;
    std::vector<Order>        orders_;
    std::vector<ProductionJob> jobs_;

    static const std::string  DATA_FILE;
};
