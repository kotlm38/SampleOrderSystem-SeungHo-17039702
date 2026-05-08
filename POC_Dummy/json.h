#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <iterator>

namespace Json {

struct Value;
using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

struct Value {
    using Variant = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
    Variant v;

    Value()                       : v(nullptr) {}
    Value(std::nullptr_t)         : v(nullptr) {}
    Value(bool b)                 : v(b) {}
    Value(int i)                  : v(static_cast<double>(i)) {}
    Value(double d)               : v(d) {}
    Value(const std::string& s)   : v(s) {}
    Value(const char* s)          : v(std::string(s)) {}
    Value(Array  a)               : v(std::move(a)) {}
    Value(Object o)               : v(std::move(o)) {}

    bool isNull()   const { return std::holds_alternative<std::nullptr_t>(v); }
    bool isBool()   const { return std::holds_alternative<bool>(v); }
    bool isNumber() const { return std::holds_alternative<double>(v); }
    bool isString() const { return std::holds_alternative<std::string>(v); }
    bool isArray()  const { return std::holds_alternative<Array>(v); }
    bool isObject() const { return std::holds_alternative<Object>(v); }

    bool               getBool()   const { return std::get<bool>(v); }
    double             getDouble() const { return std::get<double>(v); }
    int                getInt()    const { return static_cast<int>(std::get<double>(v)); }
    const std::string& getString() const { return std::get<std::string>(v); }
    const Array&       getArray()  const { return std::get<Array>(v); }
    Array&             getArray()        { return std::get<Array>(v); }
    const Object&      getObject() const { return std::get<Object>(v); }
    Object&            getObject()       { return std::get<Object>(v); }

    Value&       operator[](const std::string& key)       { return getObject()[key]; }
    const Value& operator[](const std::string& key) const { return getObject().at(key); }
    Value&       operator[](size_t i)                     { return getArray()[i]; }
    const Value& operator[](size_t i)               const { return getArray()[i]; }

    bool   has(const std::string& key) const { return isObject() && getObject().count(key) > 0; }
    size_t size() const {
        if (isArray())  return getArray().size();
        if (isObject()) return getObject().size();
        return 0;
    }
};

// ─── Serializer ──────────────────────────────────────────────────────────────

inline std::string escapeStr(const std::string& s) {
    std::string r;
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            default:   r += c;
        }
    }
    return r;
}

inline std::string serialize(const Value& val, int indent = 0, int step = 4) {
    std::string pad(indent, ' ');
    std::string inner(indent + step, ' ');

    if (val.isNull())   return "null";
    if (val.isBool())   return val.getBool() ? "true" : "false";
    if (val.isNumber()) {
        double d = val.getDouble();
        if (d == static_cast<long long>(d))
            return std::to_string(static_cast<long long>(d));
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }
    if (val.isString()) return "\"" + escapeStr(val.getString()) + "\"";

    if (val.isArray()) {
        const auto& arr = val.getArray();
        if (arr.empty()) return "[]";
        std::string r = "[\n";
        for (size_t i = 0; i < arr.size(); ++i) {
            r += inner + serialize(arr[i], indent + step, step);
            if (i + 1 < arr.size()) r += ",";
            r += "\n";
        }
        return r + pad + "]";
    }

    if (val.isObject()) {
        const auto& obj = val.getObject();
        if (obj.empty()) return "{}";
        std::string r = "{\n";
        size_t i = 0;
        for (const auto& [k, v2] : obj) {
            r += inner + "\"" + escapeStr(k) + "\": " + serialize(v2, indent + step, step);
            if (++i < obj.size()) r += ",";
            r += "\n";
        }
        return r + pad + "}";
    }
    return "null";
}

// ─── Parser ──────────────────────────────────────────────────────────────────

class Parser {
    const std::string& src;
    size_t pos = 0;

    void skipWS() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) ++pos;
    }
    char peek() { skipWS(); return pos < src.size() ? src[pos] : '\0'; }

    void expect(char c) {
        skipWS();
        if (pos >= src.size() || src[pos] != c)
            throw std::runtime_error(std::string("JSON: expected '") + c + "' at pos " + std::to_string(pos));
        ++pos;
    }

    std::string parseString() {
        expect('"');
        std::string r;
        while (pos < src.size() && src[pos] != '"') {
            if (src[pos] == '\\') {
                ++pos;
                if (pos >= src.size()) break;
                switch (src[pos]) {
                    case '"':  r += '"';  break;
                    case '\\': r += '\\'; break;
                    case '/':  r += '/';  break;
                    case 'n':  r += '\n'; break;
                    case 'r':  r += '\r'; break;
                    case 't':  r += '\t'; break;
                    default:   r += src[pos];
                }
            } else {
                r += src[pos];
            }
            ++pos;
        }
        expect('"');
        return r;
    }

    Value parseNumber() {
        skipWS();
        size_t start = pos;
        if (src[pos] == '-') ++pos;
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        if (pos < src.size() && src[pos] == '.') {
            ++pos;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        }
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            ++pos;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) ++pos;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        }
        return Value(std::stod(src.substr(start, pos - start)));
    }

    Value parseArray() {
        expect('[');
        Array arr;
        if (peek() == ']') { ++pos; return Value(std::move(arr)); }
        while (true) {
            arr.push_back(parseValue());
            if (peek() == ']') { ++pos; break; }
            expect(',');
        }
        return Value(std::move(arr));
    }

    Value parseObject() {
        expect('{');
        Object obj;
        if (peek() == '}') { ++pos; return Value(std::move(obj)); }
        while (true) {
            skipWS();
            std::string key = parseString();
            expect(':');
            obj[key] = parseValue();
            if (peek() == '}') { ++pos; break; }
            expect(',');
        }
        return Value(std::move(obj));
    }

    Value parseValue() {
        char c = peek();
        if (c == '"')                                            return Value(parseString());
        if (c == '[')                                            return parseArray();
        if (c == '{')                                            return parseObject();
        if (c == 't') { pos += 4; return Value(true); }
        if (c == 'f') { pos += 5; return Value(false); }
        if (c == 'n') { pos += 4; return Value(nullptr); }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();
        throw std::runtime_error(std::string("JSON: unexpected '") + c + "' at pos " + std::to_string(pos));
    }

public:
    explicit Parser(const std::string& s) : src(s) {}
    Value parse() { return parseValue(); }
};

inline Value parse(const std::string& s) { return Parser(s).parse(); }

// ─── File I/O ─────────────────────────────────────────────────────────────────

inline Value loadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return Value(Array{});
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (content.empty()) return Value(Array{});
    return parse(content);
}

inline bool saveFile(const std::string& path, const Value& val) {
    std::ofstream f(path);
    if (!f) return false;
    f << serialize(val, 0, 4);
    return true;
}

} // namespace Json
