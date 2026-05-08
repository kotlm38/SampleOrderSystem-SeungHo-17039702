#include "DataStore.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>

// TODO: nlohmann/json 또는 동등한 JSON 라이브러리 추가 후 구현
// #include <nlohmann/json.hpp>

const std::string DataStore::DATA_FILE = "data.json";

DataStore& DataStore::instance() {
    static DataStore inst;
    return inst;
}

void DataStore::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    // TODO: data.json 파싱 후 samples_, orders_, jobs_ 채우기
}

void DataStore::save() {
    std::lock_guard<std::mutex> lock(mutex_);
    // TODO: samples_, orders_, jobs_ → JSON 직렬화 → data.json 저장
}

// --- Sample ---

std::vector<Sample> DataStore::getSamples() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_;
}

std::optional<Sample> DataStore::findSample(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(samples_.begin(), samples_.end(),
        [&](const Sample& s) { return s.id == id; });
    if (it != samples_.end()) return *it;
    return std::nullopt;
}

void DataStore::addSample(const Sample& sample) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.push_back(sample);
    }
    save();
}

void DataStore::updateSample(const Sample& sample) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(samples_.begin(), samples_.end(),
            [&](const Sample& s) { return s.id == sample.id; });
        if (it != samples_.end()) *it = sample;
    }
    save();
}

// --- Order ---

std::vector<Order> DataStore::getOrders() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return orders_;
}

std::optional<Order> DataStore::findOrder(const std::string& orderId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(orders_.begin(), orders_.end(),
        [&](const Order& o) { return o.orderId == orderId; });
    if (it != orders_.end()) return *it;
    return std::nullopt;
}

void DataStore::addOrder(const Order& order) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        orders_.push_back(order);
    }
    save();
}

void DataStore::updateOrder(const Order& order) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(orders_.begin(), orders_.end(),
            [&](const Order& o) { return o.orderId == order.orderId; });
        if (it != orders_.end()) *it = order;
    }
    save();
}

void DataStore::removeOrder(const std::string& orderId) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        orders_.erase(std::remove_if(orders_.begin(), orders_.end(),
            [&](const Order& o) { return o.orderId == orderId; }), orders_.end());
    }
    save();
}

// --- ProductionJob ---

std::vector<ProductionJob> DataStore::getProductionJobs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_;
}

void DataStore::addProductionJob(const ProductionJob& job) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.push_back(job);
    }
    save();
}

void DataStore::removeProductionJob(const std::string& jobId) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(),
            [&](const ProductionJob& j) { return j.jobId == jobId; }), jobs_.end());
    }
    save();
}
