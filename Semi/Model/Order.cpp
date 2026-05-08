#include "Order.h"

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::Reserved:  return "Reserved";
        case OrderStatus::Rejected:  return "Rejected";
        case OrderStatus::Producing: return "Producing";
        case OrderStatus::Confirmed: return "Confirmed";
        case OrderStatus::Release:   return "Release";
        default:                     return "Unknown";
    }
}

OrderStatus orderStatusFromString(const std::string& str) {
    if (str == "Reserved")  return OrderStatus::Reserved;
    if (str == "Rejected")  return OrderStatus::Rejected;
    if (str == "Producing") return OrderStatus::Producing;
    if (str == "Confirmed") return OrderStatus::Confirmed;
    if (str == "Release")   return OrderStatus::Release;
    return OrderStatus::Reserved;
}
