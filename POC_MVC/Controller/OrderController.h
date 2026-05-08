#pragma once
#include "../Model/Order.h"
#include <vector>
#include <string>

class OrderController {
public:
    // 고객 주문 접수 → Reserved 상태로 생성
    std::string          createOrder(const std::string& sampleId, int quantity);

    // 생산담당자: 주문 승인
    //   재고 충분 → Confirmed (재고 차감)
    //   재고 부족 → Producing (생산 큐에 추가)
    bool                 approveOrder(const std::string& orderId);

    // 생산담당자: 주문 거절 → DataStore에서 삭제
    bool                 rejectOrder(const std::string& orderId);

    // Reserved 상태 주문 목록 조회
    std::vector<Order>   getReservedOrders() const;

private:
    std::string          generateOrderId() const;
};
