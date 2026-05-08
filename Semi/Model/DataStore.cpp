#include "DataStore.h"
#include "../Lib/json.hpp"
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <cmath>

using json = nlohmann::json;

DataStore& DataStore::instance() {
    static DataStore inst;
    return inst;
}

std::string DataStore::dbFilePath() {
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string exe(buf);
    auto pos = exe.rfind('\\');
    std::string dir = (pos != std::string::npos) ? exe.substr(0, pos) : ".";
    return dir + "\\..\\DB\\data.json";
}

static void ensureDbDir(const std::string& filePath) {
    auto pos = filePath.rfind('\\');
    if (pos == std::string::npos) return;
    std::string dir = filePath.substr(0, pos);
    CreateDirectoryA(dir.c_str(), nullptr);
}

void DataStore::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream ifs(dbFilePath());
    if (!ifs.is_open()) return;

    json j = json::parse(ifs, nullptr, false);
    if (j.is_discarded()) return;

    for (auto& s : j.value("samples", json::array())) {
        samples_.push_back({
            s["id"].get<std::string>(),
            s["name"].get<std::string>(),
            s["avgProductionTimeSec"].get<int>(),
            s["yield"].get<float>(),
            s["stock"].get<int>()
        });
    }

    for (auto& o : j.value("orders", json::array())) {
        Order ord;
        ord.orderId   = o["orderId"].get<std::string>();
        ord.customer  = o["customer"].get<std::string>();
        ord.sampleId  = o["sampleId"].get<std::string>();
        ord.quantity  = o["quantity"].get<int>();
        ord.status    = orderStatusFromString(o["status"].get<std::string>());
        ord.createdAt = o["createdAt"].get<std::time_t>();
        ord.updatedAt = o["updatedAt"].get<std::time_t>();
        orders_.push_back(ord);
    }

    for (auto& job : j.value("productionJobs", json::array())) {
        ProductionJob pj;
        pj.jobId       = job["jobId"].get<std::string>();
        pj.orderId     = job["orderId"].get<std::string>();
        pj.sampleId    = job["sampleId"].get<std::string>();
        pj.quantity    = job["quantity"].get<int>();
        pj.shortfall   = job["shortfall"].get<int>();
        pj.startTime   = job["startTime"].get<std::time_t>();
        pj.durationSec = job["durationSec"].get<int>();
        jobs_.push_back(pj);
    }
}

void DataStore::saveUnlocked() const {
    json j;

    json samplesArr = json::array();
    for (const auto& s : samples_) {
        samplesArr.push_back({
            {"id",                   s.id},
            {"name",                 s.name},
            {"avgProductionTimeSec", s.avgProductionTimeSec},
            {"yield",                s.yield},
            {"stock",                s.stock}
        });
    }
    j["samples"] = samplesArr;

    json ordersArr = json::array();
    for (const auto& o : orders_) {
        ordersArr.push_back({
            {"orderId",   o.orderId},
            {"customer",  o.customer},
            {"sampleId",  o.sampleId},
            {"quantity",  o.quantity},
            {"status",    orderStatusToString(o.status)},
            {"createdAt", o.createdAt},
            {"updatedAt", o.updatedAt}
        });
    }
    j["orders"] = ordersArr;

    json jobsArr = json::array();
    for (const auto& pj : jobs_) {
        jobsArr.push_back({
            {"jobId",       pj.jobId},
            {"orderId",     pj.orderId},
            {"sampleId",    pj.sampleId},
            {"quantity",    pj.quantity},
            {"shortfall",   pj.shortfall},
            {"startTime",   pj.startTime},
            {"durationSec", pj.durationSec}
        });
    }
    j["productionJobs"] = jobsArr;

    std::string target = dbFilePath();
    std::string tmp    = target + ".tmp";

    ensureDbDir(target);

    {
        std::ofstream ofs(tmp);
        ofs << j.dump(2);
    }

    MoveFileExA(tmp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING);
}

void DataStore::save() {
    std::lock_guard<std::mutex> lock(mutex_);
    saveUnlocked();
}

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

void DataStore::addSample(const Sample& s) {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.push_back(s);
    saveUnlocked();
}

void DataStore::updateSample(const Sample& s) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(samples_.begin(), samples_.end(),
        [&](const Sample& x) { return x.id == s.id; });
    if (it != samples_.end()) *it = s;
    saveUnlocked();
}

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

void DataStore::addOrder(const Order& o) {
    std::lock_guard<std::mutex> lock(mutex_);
    orders_.push_back(o);
    saveUnlocked();
}

void DataStore::updateOrder(const Order& o) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(orders_.begin(), orders_.end(),
        [&](const Order& x) { return x.orderId == o.orderId; });
    if (it != orders_.end()) *it = o;
    saveUnlocked();
}

void DataStore::removeOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    orders_.erase(std::remove_if(orders_.begin(), orders_.end(),
        [&](const Order& o) { return o.orderId == orderId; }), orders_.end());
    saveUnlocked();
}

std::vector<ProductionJob> DataStore::getProductionJobs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_;
}

void DataStore::addProductionJob(const ProductionJob& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    jobs_.push_back(job);
    saveUnlocked();
}

void DataStore::updateProductionJob(const ProductionJob& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [&](const ProductionJob& j) { return j.jobId == job.jobId; });
    if (it != jobs_.end()) *it = job;
    saveUnlocked();
}

void DataStore::removeProductionJob(const std::string& jobId) {
    std::lock_guard<std::mutex> lock(mutex_);
    jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(),
        [&](const ProductionJob& j) { return j.jobId == jobId; }), jobs_.end());
    saveUnlocked();
}
