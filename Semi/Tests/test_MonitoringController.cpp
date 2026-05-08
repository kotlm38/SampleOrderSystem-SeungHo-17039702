// test_MonitoringController.cpp
// MonitoringController unit tests

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Model/DataStore.h"
#include "../Controller/SampleController.h"
#include "../Controller/OrderController.h"
#include "../Controller/MonitoringController.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;

class MonitoringControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DataStore::instance().resetForTest();
        sampleCtrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    }
    void TearDown() override {
        DataStore::instance().resetForTest();
    }

    SampleController     sampleCtrl_;
    OrderController      orderCtrl_;
    MonitoringController monCtrl_;
};

// -----------------------------------------------------------------------
// getOrdersByStatus
// -----------------------------------------------------------------------

TEST_F(MonitoringControllerTest, GetOrdersByStatus_Reserved) {
    orderCtrl_.createOrder("A", "S001", 1);
    orderCtrl_.createOrder("B", "S001", 2);

    auto result = monCtrl_.getOrdersByStatus(OrderStatus::Reserved);
    ASSERT_THAT(result, SizeIs(2));
    for (const auto& o : result) {
        EXPECT_EQ(o.status, OrderStatus::Reserved);
    }
}

TEST_F(MonitoringControllerTest, GetOrdersByStatus_Confirmed_Empty) {
    orderCtrl_.createOrder("A", "S001", 1);

    auto result = monCtrl_.getOrdersByStatus(OrderStatus::Confirmed);
    EXPECT_THAT(result, IsEmpty());
}

TEST_F(MonitoringControllerTest, GetOrdersByStatus_MixedStatuses_Filtered) {
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 100;
    DataStore::instance().updateSample(s);

    std::string id1 = orderCtrl_.createOrder("A", "S001", 1);
    std::string id2 = orderCtrl_.createOrder("B", "S001", 2);
    std::string id3 = orderCtrl_.createOrder("C", "S001", 3);

    orderCtrl_.approveOrder(id3);

    auto reserved  = monCtrl_.getOrdersByStatus(OrderStatus::Reserved);
    auto confirmed = monCtrl_.getOrdersByStatus(OrderStatus::Confirmed);

    ASSERT_THAT(reserved,  SizeIs(2));
    ASSERT_THAT(confirmed, SizeIs(1));
    EXPECT_EQ(confirmed[0].orderId, id3);
}

TEST_F(MonitoringControllerTest, GetOrdersByStatus_Producing_Filtered) {
    std::string id = orderCtrl_.createOrder("A", "S001", 5);
    orderCtrl_.approveOrder(id);

    auto producing = monCtrl_.getOrdersByStatus(OrderStatus::Producing);
    ASSERT_THAT(producing, SizeIs(1));
    EXPECT_EQ(producing[0].status, OrderStatus::Producing);
}

// -----------------------------------------------------------------------
// getInventoryStatus (StockStatus)
// -----------------------------------------------------------------------

TEST_F(MonitoringControllerTest, StockStatus_Sufficient_NoOrders) {
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 100;
    DataStore::instance().updateSample(s);

    auto inv = monCtrl_.getInventoryStatus();
    ASSERT_THAT(inv, SizeIs(1));
    EXPECT_EQ(inv[0].stockStatus, StockStatus::Sufficient);
}

TEST_F(MonitoringControllerTest, StockStatus_Depleted_WhenStockZero) {
    auto inv = monCtrl_.getInventoryStatus();
    ASSERT_THAT(inv, SizeIs(1));
    EXPECT_EQ(inv[0].stockStatus, StockStatus::Depleted);
}

TEST_F(MonitoringControllerTest, StockStatus_Shortage) {
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 2;
    DataStore::instance().updateSample(s);

    orderCtrl_.createOrder("A", "S001", 5);

    auto inv = monCtrl_.getInventoryStatus();
    ASSERT_THAT(inv, SizeIs(1));
    EXPECT_EQ(inv[0].stockStatus, StockStatus::Shortage);
}

TEST_F(MonitoringControllerTest, StockStatus_Sufficient_WhenStockMeetsDemand) {
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 10;
    DataStore::instance().updateSample(s);

    orderCtrl_.createOrder("A", "S001", 10);

    auto inv = monCtrl_.getInventoryStatus();
    ASSERT_THAT(inv, SizeIs(1));
    EXPECT_EQ(inv[0].stockStatus, StockStatus::Sufficient);
}

TEST_F(MonitoringControllerTest, InventoryStatus_MultipleSamples) {
    sampleCtrl_.registerSample("S002", "Chip B", 5, 0.8f);

    auto inv = monCtrl_.getInventoryStatus();
    ASSERT_THAT(inv, SizeIs(2));
}

TEST_F(MonitoringControllerTest, Inventory_Depleted_WhenStockZeroAndDemandExists) {
    orderCtrl_.createOrder("A", "S001", 3);

    auto inv = monCtrl_.getInventoryStatus();
    ASSERT_THAT(inv, SizeIs(1));
    EXPECT_EQ(inv[0].stockStatus, StockStatus::Depleted);
}
