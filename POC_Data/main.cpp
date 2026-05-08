#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <windows.h>
#include "json.h"

const std::string DATA_FILE = "samples.json";

// ─── Model ────────────────────────────────────────────────────────────────────

struct Sample {
    int         id;
    std::string name;
    std::string type;       // wafer / chip / substrate
    int         quantity;
    std::string status;     // pending / in_production / completed / shipped
    std::string createdAt;
};

// ─── Helpers ──────────────────────────────────────────────────────────────────

std::string nowString() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

Json::Value toJson(const Sample& s) {
    Json::Object o;
    o["id"]         = Json::Value(s.id);
    o["name"]       = Json::Value(s.name);
    o["type"]       = Json::Value(s.type);
    o["quantity"]   = Json::Value(s.quantity);
    o["status"]     = Json::Value(s.status);
    o["created_at"] = Json::Value(s.createdAt);
    return Json::Value(std::move(o));
}

Sample fromJson(const Json::Value& v) {
    return {
        v["id"].getInt(),
        v["name"].getString(),
        v["type"].getString(),
        v["quantity"].getInt(),
        v["status"].getString(),
        v["created_at"].getString()
    };
}

Json::Value loadAll() {
    return Json::loadFile(DATA_FILE);
}

void saveAll(const Json::Value& arr) {
    if (!Json::saveFile(DATA_FILE, arr))
        std::cerr << "[오류] 파일 저장 실패: " << DATA_FILE << "\n";
}

int nextId(const Json::Value& arr) {
    int maxId = 0;
    for (size_t i = 0; i < arr.size(); ++i) {
        int id = arr[i]["id"].getInt();
        if (id > maxId) maxId = id;
    }
    return maxId + 1;
}

void printSample(const Sample& s) {
    std::cout << "  [" << s.id << "] "
              << s.name
              << " | 유형: " << s.type
              << " | 수량: " << s.quantity
              << " | 상태: " << s.status
              << " | 등록일시: " << s.createdAt << "\n";
}

// ─── CRUD ─────────────────────────────────────────────────────────────────────

void create() {
    std::cout << "\n=== 시료 등록 (Create) ===\n";

    Sample s{};
    std::cout << "이름        : "; std::getline(std::cin, s.name);
    std::cout << "유형 (wafer/chip/substrate): "; std::getline(std::cin, s.type);
    std::cout << "수량        : ";
    std::cin >> s.quantity;
    std::cin.ignore();

    s.status    = "pending";
    s.createdAt = nowString();

    auto arr = loadAll();
    s.id = nextId(arr);
    arr.getArray().push_back(toJson(s));
    saveAll(arr);

    std::cout << "[완료] 시료 등록됨 → ID: " << s.id << "\n";
}

void readAll() {
    auto arr = loadAll();
    std::cout << "\n=== 시료 목록 (Read) ? " << arr.size() << "건 ===\n";
    if (arr.size() == 0) {
        std::cout << "  등록된 시료 없음\n";
        return;
    }
    for (size_t i = 0; i < arr.size(); ++i)
        printSample(fromJson(arr[i]));
}

void update() {
    readAll();
    if (loadAll().size() == 0) return;

    std::cout << "\n수정할 시료 ID: ";
    int id; std::cin >> id; std::cin.ignore();

    auto arr = loadAll();
    bool found = false;
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i]["id"].getInt() != id) continue;
        found = true;

        auto& item = arr.getArray()[i];

        std::cout << "새 수량   (현재: " << item["quantity"].getInt() << ", 변경 없으면 -1): ";
        int qty; std::cin >> qty; std::cin.ignore();

        std::cout << "새 상태   (현재: " << item["status"].getString() << ")\n"
                  << "  선택 → pending / in_production / completed / shipped (변경 없으면 Enter): ";
        std::string status; std::getline(std::cin, status);

        if (qty >= 0)          item["quantity"] = Json::Value(qty);
        if (!status.empty())   item["status"]   = Json::Value(status);
        break;
    }

    if (!found) { std::cout << "[오류] ID " << id << " 없음\n"; return; }
    saveAll(arr);
    std::cout << "[완료] ID " << id << " 수정됨\n";
}

void remove() {
    readAll();
    if (loadAll().size() == 0) return;

    std::cout << "\n삭제할 시료 ID: ";
    int id; std::cin >> id; std::cin.ignore();

    auto arr = loadAll();
    auto& vec = arr.getArray();
    auto it = std::find_if(vec.begin(), vec.end(),
        [id](const Json::Value& v) { return v["id"].getInt() == id; });

    if (it == vec.end()) { std::cout << "[오류] ID " << id << " 없음\n"; return; }

    std::cout << "정말 삭제하시겠습니까? [y/N]: ";
    std::string confirm; std::getline(std::cin, confirm);
    if (confirm != "y" && confirm != "Y") { std::cout << "취소됨\n"; return; }

    vec.erase(it);
    saveAll(arr);
    std::cout << "[완료] ID " << id << " 삭제됨\n";
}

// ─── Entry ────────────────────────────────────────────────────────────────────

int main() {
    //SetConsoleOutputCP(CP_UTF8);
    //SetConsoleCP(CP_UTF8);

    std::cout << "데이터 파일: " << DATA_FILE << "\n";

    while (true) {
        std::cout << "\n============================\n"
                  << " 시료 관리  (JSON CRUD POC)\n"
                  << "============================\n"
                  << " 1. 시료 등록  (Create)\n"
                  << " 2. 목록 조회  (Read)\n"
                  << " 3. 시료 수정  (Update)\n"
                  << " 4. 시료 삭제  (Delete)\n"
                  << " 0. 종료\n"
                  << "선택 >> ";

        int choice = -1;
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
            case 1: create(); break;
            case 2: readAll(); break;
            case 3: update(); break;
            case 4: remove(); break;
            case 0: std::cout << "종료합니다.\n"; return 0;
            default: std::cout << "[오류] 잘못된 입력\n";
        }
    }
}
