#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <windows.h>
#include "json.h"

// DB/data.json 위치를 실행 경로에 따라 자동 탐색
static std::string findDataFile() {
    static const std::vector<std::string> candidates = {
        "DB/data.json",
        "../DB/data.json",
        "../../DB/data.json",
        "../../../DB/data.json"
    };
    for (const auto& p : candidates) {
        std::ifstream f(p);
        if (f.good()) return p;
    }
    return candidates[0];
}

static std::string todayString() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d");
    return oss.str();
}

// 오늘 날짜 기준 마지막 주문 시퀀스 번호 반환
static int maxOrderSeq(const Json::Value& orders, const std::string& today) {
    int maxSeq = 0;
    std::string prefix = "ORD-" + today + "-";
    for (size_t i = 0; i < orders.size(); ++i) {
        if (!orders[i].has("orderId")) continue;
        const std::string& oid = orders[i]["orderId"].getString();
        if (oid.size() <= prefix.size() || oid.rfind(prefix, 0) != 0) continue;
        try {
            int seq = std::stoi(oid.substr(prefix.size()));
            if (seq > maxSeq) maxSeq = seq;
        } catch (...) {}
    }
    return maxSeq;
}

static std::string makeOrderId(const std::string& today, int seq) {
    std::ostringstream oss;
    oss << "ORD-" << today << "-" << std::setfill('0') << std::setw(3) << seq;
    return oss.str();
}

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    //SetConsoleCP(CP_UTF8);

    std::string dataFile = findDataFile();
    std::cout << "데이터 파일: " << dataFile << "\n";

    Json::Value root = Json::loadFile(dataFile);

    // 파일이 없거나 비어있으면 Object로 초기화
    if (!root.isObject())
        root = Json::Value(Json::Object{});
    if (!root.has("samples"))        root["samples"]        = Json::Value(Json::Array{});
    if (!root.has("orders"))         root["orders"]         = Json::Value(Json::Array{});
    if (!root.has("productionJobs")) root["productionJobs"] = Json::Value(Json::Array{});

    // 시료 카탈로그에서 ID 목록 수집
    std::vector<std::string> sampleIds;
    for (size_t i = 0; i < root["samples"].size(); ++i)
        sampleIds.push_back(root["samples"][i]["id"].getString());
    if (sampleIds.empty())
        sampleIds = { "S001", "S002", "S003" };

    // 상태 가중 풀 (Reserved가 가장 많이 생성되도록)
    static const std::vector<std::string> statusPool = {
        "Reserved",  "Reserved",  "Reserved",
        "Producing", "Producing",
        "Confirmed",
        "Release"
    };

    std::string today = todayString();
    int seq = maxOrderSeq(root["orders"], today);

    std::time_t now = std::time(nullptr);
    std::srand(static_cast<unsigned>(now));

    std::cout << "\n더미 주문 10건 생성 중...\n\n";

    for (int i = 0; i < 10; ++i) {
        ++seq;
        std::string orderId  = makeOrderId(today, seq);
        std::string sampleId = sampleIds[std::rand() % sampleIds.size()];
        int         quantity = 1 + std::rand() % 20;
        std::string status   = statusPool[std::rand() % statusPool.size()];
        int createdAt = static_cast<int>(now) - (std::rand() % 3600);
        int updatedAt = createdAt + (std::rand() % 1800);

        Json::Object order;
        order["orderId"]   = Json::Value(orderId);
        order["sampleId"]  = Json::Value(sampleId);
        order["quantity"]  = Json::Value(quantity);
        order["status"]    = Json::Value(status);
        order["createdAt"] = Json::Value(createdAt);
        order["updatedAt"] = Json::Value(updatedAt);
        root["orders"].getArray().push_back(Json::Value(std::move(order)));

        std::cout << "  [" << std::setw(2) << (i + 1) << "] "
                  << orderId
                  << " | " << sampleId
                  << " | 수량: " << std::setw(2) << quantity
                  << " | 상태: " << status << "\n";
    }

    if (!Json::saveFile(dataFile, root)) {
        std::cerr << "\n[오류] 파일 저장 실패: " << dataFile << "\n";
        return 1;
    }

    std::cout << "\n[완료] " << dataFile
              << "  (총 주문: " << root["orders"].size() << "건)\n";
    return 0;
}
