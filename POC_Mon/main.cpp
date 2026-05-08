#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <iomanip>
#include <algorithm>
#define NOMINMAX
#include <Windows.h>
#include <conio.h>

// ── ANSI color helpers ───────────────────────────────────────────────────────
namespace clr {
    constexpr const char* reset  = "\033[0m";
    constexpr const char* bold   = "\033[1m";
    constexpr const char* red    = "\033[31m";
    constexpr const char* green  = "\033[32m";
    constexpr const char* yellow = "\033[33m";
    constexpr const char* cyan   = "\033[36m";
    constexpr const char* white  = "\033[97m";
    constexpr const char* gray   = "\033[90m";
    constexpr const char* magenta = "\033[35m";
}

// ── Minimal JSON parser ──────────────────────────────────────────────────────
namespace json {
    using Obj = std::map<std::string, std::string>;

    static void ws(const std::string& s, size_t& i) {
        while (i < s.size() && (unsigned char)s[i] <= ' ') ++i;
    }

    // Returns unescaped string content (without quotes). i points at opening ".
    static std::string parseStr(const std::string& s, size_t& i) {
        ++i; // skip opening "
        std::string r;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) { ++i; r += s[i++]; }
            else r += s[i++];
        }
        if (i < s.size()) ++i; // skip closing "
        return r;
    }

    // Captures raw JSON value text (string kept with quotes, others as-is).
    static std::string raw(const std::string& s, size_t& i) {
        ws(s, i);
        if (i >= s.size()) return "";
        size_t start = i;
        char c = s[i];
        if (c == '"') {
            std::string v = parseStr(s, i);
            return '"' + v + '"';
        }
        if (c == '{' || c == '[') {
            char open = c, close = (c == '{') ? '}' : ']';
            int depth = 0;
            while (i < s.size()) {
                if (s[i] == '"') { parseStr(s, i); continue; }
                if (s[i] == open) ++depth;
                else if (s[i] == close) { --depth; ++i; if (depth == 0) break; continue; }
                ++i;
            }
            return s.substr(start, i - start);
        }
        // number / bool / null
        while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' &&
               (unsigned char)s[i] > ' ') ++i;
        return s.substr(start, i - start);
    }

    // Parses an object into key → raw-value map. i points at '{'.
    static Obj obj(const std::string& s, size_t& i) {
        Obj o;
        ++i; // skip {
        ws(s, i);
        if (i < s.size() && s[i] == '}') { ++i; return o; }
        while (i < s.size()) {
            ws(s, i);
            if (i >= s.size() || s[i] != '"') break;
            std::string key = parseStr(s, i);
            ws(s, i);
            if (i < s.size() && s[i] == ':') ++i;
            ws(s, i);
            o[key] = raw(s, i);
            ws(s, i);
            if (i < s.size() && s[i] == ',') ++i; else break;
        }
        if (i < s.size() && s[i] == '}') ++i;
        return o;
    }

    // Parses an array of objects. i points at '['.
    static std::vector<Obj> arr(const std::string& s, size_t& i) {
        std::vector<Obj> result;
        ++i; // skip [
        ws(s, i);
        if (i < s.size() && s[i] == ']') { ++i; return result; }
        while (i < s.size()) {
            ws(s, i);
            if (i >= s.size()) break;
            if (s[i] == '{') result.push_back(obj(s, i));
            else raw(s, i); // skip non-object element
            ws(s, i);
            if (i < s.size() && s[i] == ',') ++i; else break;
        }
        if (i < s.size() && s[i] == ']') ++i;
        return result;
    }

    std::string getStr(const Obj& o, const std::string& key, const std::string& def = "") {
        auto it = o.find(key);
        if (it == o.end()) return def;
        const auto& v = it->second;
        if (v.size() >= 2 && v.front() == '"') return v.substr(1, v.size() - 2);
        return v;
    }

    int getInt(const Obj& o, const std::string& key, int def = 0) {
        auto it = o.find(key);
        if (it == o.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    }

    double getDbl(const Obj& o, const std::string& key, double def = 0.0) {
        auto it = o.find(key);
        if (it == o.end()) return def;
        try { return std::stod(it->second); } catch (...) { return def; }
    }

    long long getLL(const Obj& o, const std::string& key, long long def = 0) {
        auto it = o.find(key);
        if (it == o.end()) return def;
        try { return std::stoll(it->second); } catch (...) { return def; }
    }
}

// ── Data structures ──────────────────────────────────────────────────────────
struct Sample {
    std::string id, name;
    int         avgTimeSec = 0;
    float       yield      = 0.f;
    int         stock      = 0;
};

struct Order {
    std::string orderId, sampleId, status;
    int         quantity  = 0;
    std::time_t createdAt = 0, updatedAt = 0;
};

struct Job {
    std::string jobId, orderId, sampleId;
    int         quantity    = 0;
    int         durationSec = 0;
    std::time_t startTime   = 0;
};

struct DB {
    std::vector<Sample> samples;
    std::vector<Order>  orders;
    std::vector<Job>    jobs;
    bool loaded = false;
};

// ── DB loader ────────────────────────────────────────────────────────────────
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string findDataFile() {
    const std::vector<std::string> candidates = {
        "../DB/data.json",
        "../../DB/data.json",
        "DB/data.json",
        "data.json",
    };
    for (auto& p : candidates) {
        std::ifstream f(p);
        if (f) return p;
    }
    return "";
}

static DB loadDB(const std::string& path) {
    DB db;
    std::string text = readFile(path);
    if (text.empty()) return db;

    size_t i = 0;
    json::ws(text, i);
    if (i >= text.size() || text[i] != '{') return db;
    auto root = json::obj(text, i);
    db.loaded = true;

    auto parseSection = [&](const std::string& key) -> std::vector<json::Obj> {
        auto it = root.find(key);
        if (it == root.end()) return {};
        size_t si = 0;
        json::ws(it->second, si);
        if (si >= it->second.size() || it->second[si] != '[') return {};
        return json::arr(it->second, si);
    };

    for (auto& o : parseSection("samples")) {
        db.samples.push_back({
            json::getStr(o, "id"),
            json::getStr(o, "name"),
            json::getInt(o, "avgProductionTimeSec"),
            (float)json::getDbl(o, "yield"),
            json::getInt(o, "stock")
        });
    }

    for (auto& o : parseSection("orders")) {
        db.orders.push_back({
            json::getStr(o, "orderId"),
            json::getStr(o, "sampleId"),
            json::getStr(o, "status"),
            json::getInt(o, "quantity"),
            (std::time_t)json::getLL(o, "createdAt"),
            (std::time_t)json::getLL(o, "updatedAt")
        });
    }

    for (auto& o : parseSection("productionJobs")) {
        db.jobs.push_back({
            json::getStr(o, "jobId"),
            json::getStr(o, "orderId"),
            json::getStr(o, "sampleId"),
            json::getInt(o, "quantity"),
            json::getInt(o, "durationSec"),
            (std::time_t)json::getLL(o, "startTime")
        });
    }

    return db;
}

// ── Display utilities ────────────────────────────────────────────────────────
static std::string nowStr() {
    auto t = std::time(nullptr);
    std::tm tm_s{};
    localtime_s(&tm_s, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_s);
    return buf;
}

static std::string fmtTime(std::time_t t) {
    if (t == 0) return "---";
    std::tm tm_s{};
    localtime_s(&tm_s, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%m-%d %H:%M:%S", &tm_s);
    return buf;
}

static std::string progressBar(int elapsed, int total, int width = 24) {
    if (total <= 0) return std::string(width, '-');
    int fill = std::min(width, std::max(0, (int)((double)elapsed / total * width)));
    return std::string(fill, '#') + std::string(width - fill, '.');
}

static const char* statusColor(const std::string& st) {
    if (st == "Reserved")  return clr::yellow;
    if (st == "Producing") return clr::cyan;
    if (st == "Confirmed") return clr::green;
    if (st == "Release")   return clr::gray;
    if (st == "Rejected")  return clr::red;
    return clr::white;
}

// Pad/right-align using byte length (acceptable for ASCII data columns).
static std::string lpad(std::string s, int w) {
    if ((int)s.size() >= w) return s.substr(0, w);
    return s + std::string(w - s.size(), ' ');
}
static std::string rpad(std::string s, int w) {
    if ((int)s.size() >= w) return s.substr(0, w);
    return std::string(w - s.size(), ' ') + s;
}

// ── Render sections ──────────────────────────────────────────────────────────
static void renderHeader(const std::string& path, bool loaded) {
    std::cout
        << clr::bold << clr::cyan
        << "╔══════════════════════════════════════════════════════════════╗\n"
        << "║           Semi Factory  -  DB Real-time Monitor              ║\n"
        << "╚══════════════════════════════════════════════════════════════╝"
        << clr::reset << "\n";
    std::cout
        << clr::gray << " DB : " << path << "\n"
        << " Time: " << nowStr()
        << "    [q] Quit   [any key] Force refresh   auto 2s"
        << clr::reset << "\n";
    if (!loaded)
        std::cout << clr::red << " ※ data.json 파일을 읽을 수 없습니다.\n" << clr::reset;
    std::cout << "\n";
}

static void renderSamples(const std::vector<Sample>& samples) {
    std::cout << clr::bold << clr::white
              << "■ 시료 목록 (" << samples.size() << ")\n" << clr::reset;
    std::cout << clr::gray
              << "  " << lpad("ID",      8)
              << "  " << lpad("Name",   18)
              << "  " << rpad("Avg(s)", 6)
              << "  " << rpad("Yield",  6)
              << "  " << rpad("Stock",  5) << "\n"
              << "  " << std::string(49, '-') << "\n" << clr::reset;

    if (samples.empty()) {
        std::cout << clr::gray << "  (none)\n" << clr::reset;
    } else {
        for (auto& s : samples) {
            std::cout
                << "  " << clr::cyan << lpad(s.id, 8) << clr::reset
                << "  " << lpad(s.name, 18)
                << "  " << clr::gray << rpad(std::to_string(s.avgTimeSec), 6) << clr::reset
                << "  " << rpad(std::to_string((int)(s.yield * 100)) + "%", 6)
                << "  " << (s.stock == 0 ? clr::red : s.stock < 5 ? clr::yellow : clr::green)
                        << rpad(std::to_string(s.stock), 5) << clr::reset
                << "\n";
        }
    }
    std::cout << "\n";
}

static void renderInventory(const std::vector<Sample>& samples,
                            const std::vector<Order>&  orders) {
    std::map<std::string, int> reserved;
    for (auto& o : orders)
        if (o.status == "Reserved" || o.status == "Producing")
            reserved[o.sampleId] += o.quantity;

    std::cout << clr::bold << clr::white << "■ 재고 현황\n" << clr::reset;
    std::cout << clr::gray
              << "  " << lpad("ID",      8)
              << "  " << lpad("Name",   18)
              << "  " << rpad("Stock",  5)
              << "  " << rpad("Ordered", 7)
              << "  " << lpad("State",   6) << "\n"
              << "  " << std::string(52, '-') << "\n" << clr::reset;

    if (samples.empty()) {
        std::cout << clr::gray << "  (none)\n" << clr::reset;
    } else {
        for (auto& s : samples) {
            int res = reserved.count(s.id) ? reserved.at(s.id) : 0;
            const char* col;
            std::string state;
            if      (s.stock == 0)       { col = clr::red;    state = "고갈"; }
            else if (s.stock < res)      { col = clr::yellow; state = "부족"; }
            else                         { col = clr::green;  state = "여유"; }
            std::cout
                << "  " << clr::cyan << lpad(s.id, 8) << clr::reset
                << "  " << lpad(s.name, 18)
                << "  " << col << rpad(std::to_string(s.stock), 5) << clr::reset
                << "  " << rpad(std::to_string(res), 7)
                << "  " << col << lpad(state, 6) << clr::reset
                << "\n";
        }
    }
    std::cout << "\n";
}

static void renderOrders(const std::vector<Order>& orders) {
    std::map<std::string, int> cnt;
    for (auto& o : orders) cnt[o.status]++;

    std::cout << clr::bold << clr::white
              << "■ 주문 현황 (total " << orders.size() << ")\n" << clr::reset;

    for (auto& st : {"Reserved", "Producing", "Confirmed", "Release"}) {
        int n = cnt.count(st) ? cnt.at(st) : 0;
        std::cout << "  " << statusColor(st) << lpad(st, 10) << clr::reset
                  << clr::bold << rpad(std::to_string(n), 4) << clr::reset;
    }
    std::cout << "\n";

    std::cout << clr::gray
              << "  " << lpad("OrderID",  14)
              << "  " << lpad("SampleID",  8)
              << "  " << rpad("Qty",       3)
              << "  " << lpad("Status",   10)
              << "  " << lpad("Created",  17)
              << "  " << lpad("Updated",  17) << "\n"
              << "  " << std::string(76, '-') << "\n" << clr::reset;

    if (orders.empty()) {
        std::cout << clr::gray << "  (none)\n" << clr::reset;
    } else {
        for (auto& o : orders) {
            std::cout
                << "  " << clr::cyan << lpad(o.orderId, 14) << clr::reset
                << "  " << lpad(o.sampleId, 8)
                << "  " << rpad(std::to_string(o.quantity), 3)
                << "  " << statusColor(o.status) << lpad(o.status, 10) << clr::reset
                << "  " << clr::gray << lpad(fmtTime(o.createdAt), 17) << clr::reset
                << "  " << clr::gray << lpad(fmtTime(o.updatedAt), 17) << clr::reset
                << "\n";
        }
    }
    std::cout << "\n";
}

static void renderJobs(const std::vector<Job>& jobs) {
    std::cout << clr::bold << clr::white
              << "■ 생산 진행 (" << jobs.size() << ")\n" << clr::reset;
    std::cout << clr::gray
              << "  " << lpad("JobID",    14)
              << "  " << lpad("SampleID",  8)
              << "  " << rpad("Qty",       3)
              << "  " << lpad("Progress",  30)
              << "  " << rpad("Remain",    7) << "\n"
              << "  " << std::string(68, '-') << "\n" << clr::reset;

    if (jobs.empty()) {
        std::cout << clr::gray << "  (no active jobs)\n" << clr::reset;
    } else {
        auto now = std::time(nullptr);
        for (auto& j : jobs) {
            int elapsed = (int)(now - j.startTime);
            int remain  = std::max(0, j.durationSec - elapsed);
            int pct     = (j.durationSec > 0)
                ? std::min(100, (int)((double)elapsed / j.durationSec * 100))
                : 100;
            std::string bar = progressBar(elapsed, j.durationSec);
            std::string pctStr = std::to_string(pct) + "%";
            const char* bcol = (pct >= 100) ? clr::green : clr::cyan;

            std::cout
                << "  " << clr::cyan << lpad(j.jobId, 14) << clr::reset
                << "  " << lpad(j.sampleId, 8)
                << "  " << rpad(std::to_string(j.quantity), 3)
                << "  " << bcol << "[" << bar << "] " << rpad(pctStr, 4) << clr::reset
                << "  " << (remain == 0 ? clr::green : clr::yellow)
                        << rpad(std::to_string(remain) + "s", 7) << clr::reset
                << "\n";
        }
    }
    std::cout << "\n";
}

// ── Entry point ──────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Enable ANSI escape codes and UTF-8
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    //SetConsoleOutputCP(CP_UTF8);
    //SetConsoleCP(CP_UTF8);

    std::string dbPath;
    if (argc >= 2) {
        dbPath = argv[1];
    } else {
        dbPath = findDataFile();
        if (dbPath.empty()) {
            std::cerr
                << "data.json 을 찾을 수 없습니다.\n"
                << "사용법: POC_Mon.exe [경로/data.json]\n"
                << "기본 탐색 경로: ../DB/data.json, ../../DB/data.json, DB/data.json, data.json\n";
            std::cin.get();
            return 1;
        }
    }

    constexpr int REFRESH_INTERVAL_MS = 2000;
    constexpr int POLL_STEP_MS        = 100;

    while (true) {
        // Clear screen and move cursor to top-left
        std::cout << "\033[2J\033[H" << std::flush;

        DB db = loadDB(dbPath);
        renderHeader(dbPath, db.loaded);
        renderSamples(db.samples);
        renderInventory(db.samples, db.orders);
        renderOrders(db.orders);
        renderJobs(db.jobs);

        // Poll for keypress up to REFRESH_INTERVAL_MS
        for (int waited = 0; waited < REFRESH_INTERVAL_MS; waited += POLL_STEP_MS) {
            if (_kbhit()) {
                int c = _getch();
                if (c == 0 || c == 0xE0) _getch(); // drain extended key
                if (c == 'q' || c == 'Q') {
                    std::cout << "\n모니터 종료.\n";
                    return 0;
                }
                break; // any other key → immediate refresh
            }
            Sleep(POLL_STEP_MS);
        }
    }
}
