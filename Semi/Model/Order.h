#pragma once
#include <string>
#include <ctime>

enum class OrderStatus {
    Reserved,
    Rejected,
    Producing,
    Confirmed,
    Release
};

std::string orderStatusToString(OrderStatus status);
OrderStatus orderStatusFromString(const std::string& str);

struct Order {
    std::string orderId;
    std::string customer;
    std::string sampleId;
    int         quantity;
    OrderStatus status;
    std::time_t createdAt;
    std::time_t updatedAt;
};
