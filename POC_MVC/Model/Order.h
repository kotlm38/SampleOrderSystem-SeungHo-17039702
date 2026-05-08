#pragma once
#include <string>
#include <ctime>

enum class OrderStatus {
    Reserved,   // 주문 접수 초기 상태
    Rejected,   // 주문 거절
    Producing,  // 재고 부족으로 생산 중
    Confirmed,  // 시료 준비 완료
    Release     // 출고 완료
};

std::string orderStatusToString(OrderStatus status);
OrderStatus orderStatusFromString(const std::string& str);

struct Order {
    std::string orderId;
    std::string sampleId;
    int         quantity;
    OrderStatus status;
    std::time_t createdAt;
    std::time_t updatedAt;
};
