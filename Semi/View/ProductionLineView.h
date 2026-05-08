#pragma once
#include <ctime>
#include <string>

class ProductionLineView {
public:
    void show() const;
private:
    static std::string formatTime(std::time_t t);
};
