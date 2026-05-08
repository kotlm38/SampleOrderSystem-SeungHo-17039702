// test_OrderController.cpp
// OrderController unit tests

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Model/DataStore.h"
#include "../Controller/SampleController.h"
#include "../Controller/OrderController.h"

using ::testing::Not;
using ::testing::IsEmpty;
using ::testing::SizeIs;

class OrderControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DataStore::instance().resetForTest();
        sampleCtrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    }
    void TearDown() override {
        DataStore::instance().resetForTest();
    }

    SampleController sampleCtrl_;
    OrderController  orderCtrl_;
};

// -----------------------------------------------------------------------
// createOrder
// -----------------------------------------------------------------------

TEST_F(OrderControllerTest, CreateOrder_ValidSample_ReturnsNonEmptyId) {
    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 5);
    EXPECT_THAT(id, Not(IsEmpty()));
}

TEST_F(OrderControllerTest, CreateOrder_InvalidSample_ReturnsEmpty) {
    std::string id = orderCtrl_.createOrder("CustomerA", "INVALID_ID", 5);
    EXPECT_THAT(id, IsEmpty());
}

TEST_F(OrderControllerTest, CreateOrder_StatusIsReserved) {
    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 3);
    ASSERT_THAT(id, Not(IsEmpty()));

    auto order = DataStore::instance().findOrder(id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->status, OrderStatus::Reserved);
}

TEST_F(OrderControllerTest, CreateOrder_UniqueIds) {
    std::string id1 = orderCtrl_.createOrder("CustomerA", "S001", 1);
    std::string id2 = orderCtrl_.createOrder("CustomerB", "S001", 2);
    EXPECT_NE(id1, id2);
}

TEST_F(OrderControllerTest, CreateOrder_FieldsStoredCorrectly) {
    std::string id = orderCtrl_.createOrder("Alice", "S001", 7);
    auto order = DataStore::instance().findOrder(id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->customer, "Alice");
    EXPECT_EQ(order->sampleId, "S001");
    EXPECT_EQ(order->quantity, 7);
}

// -----------------------------------------------------------------------
// getReservedOrders
// -----------------------------------------------------------------------

TEST_F(OrderControllerTest, GetReservedOrders_ReturnsOnlyReserved) {
    orderCtrl_.createOrder("A", "S001", 1);
    orderCtrl_.createOrder("B", "S001", 2);

    auto reserved = orderCtrl_.getReservedOrders();
    ASSERT_THAT(reserved, SizeIs(2));
    for (const auto& o : reserved) {
        EXPECT_EQ(o.status, OrderStatus::Reserved);
    }
}

TEST_F(OrderControllerTest, GetReservedOrders_ExcludesNonReserved) {
    // Set stock so approve results in Confirmed
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 100;
    DataStore::instance().updateSample(s);

    std::string id = orderCtrl_.createOrder("A", "S001", 1);
    orderCtrl_.approveOrder(id);

    auto reserved = orderCtrl_.getReservedOrders();
    EXPECT_THAT(reserved, IsEmpty());
}

// -----------------------------------------------------------------------
// approveOrder - sufficient stock
// -----------------------------------------------------------------------

TEST_F(OrderControllerTest, ApproveOrder_WithStock_StatusConfirmed) {
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 10;
    DataStore::instance().updateSample(s);

    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 3);
    bool ok = orderCtrl_.approveOrder(id);
    EXPECT_TRUE(ok);

    auto order = DataStore::instance().findOrder(id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->status, OrderStatus::Confirmed);
}

TEST_F(OrderControllerTest, ApproveOrder_WithStock_StockDecremented) {
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 10;
    DataStore::instance().updateSample(s);

    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 3);
    orderCtrl_.approveOrder(id);

    auto updated = DataStore::instance().findSample("S001");
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->stock, 7);
}

// -----------------------------------------------------------------------
// approveOrder - insufficient stock -> production
// -----------------------------------------------------------------------

TEST_F(OrderControllerTest, ApproveOrder_NoStock_StatusProducing) {
    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 5);
    bool ok = orderCtrl_.approveOrder(id);
    EXPECT_TRUE(ok);

    auto order = DataStore::instance().findOrder(id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->status, OrderStatus::Producing);
}

TEST_F(OrderControllerTest, ApproveOrder_NoStock_ProductionJobCreated) {
    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 5);
    orderCtrl_.approveOrder(id);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));
    EXPECT_EQ(jobs[0].orderId, id);
}

TEST_F(OrderControllerTest, ApproveOrder_PartialStock_ShortfallCalculated) {
    // stock=2, order=5 -> shortfall=3
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 2;
    DataStore::instance().updateSample(s);

    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 5);
    orderCtrl_.approveOrder(id);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));
    EXPECT_EQ(jobs[0].shortfall, 3);
}

TEST_F(OrderControllerTest, ApproveOrder_InvalidOrderId_ReturnsFalse) {
    bool ok = orderCtrl_.approveOrder("NON-EXISTENT-ORDER");
    EXPECT_FALSE(ok);
}

// -----------------------------------------------------------------------
// rejectOrder
// -----------------------------------------------------------------------

TEST_F(OrderControllerTest, RejectOrder_OrderRemovedFromStore) {
    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 3);
    ASSERT_THAT(id, Not(IsEmpty()));

    bool ok = orderCtrl_.rejectOrder(id);
    EXPECT_TRUE(ok);

    auto order = DataStore::instance().findOrder(id);
    EXPECT_FALSE(order.has_value());
}

TEST_F(OrderControllerTest, RejectOrder_InvalidId_ReturnsFalse) {
    bool ok = orderCtrl_.rejectOrder("NON-EXISTENT");
    EXPECT_FALSE(ok);
}

TEST_F(OrderControllerTest, RejectOrder_OrderListBecomesEmpty) {
    std::string id = orderCtrl_.createOrder("CustomerA", "S001", 1);
    orderCtrl_.rejectOrder(id);
    EXPECT_THAT(DataStore::instance().getOrders(), IsEmpty());
}
