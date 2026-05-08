// test_ShippingController.cpp
// ShippingController unit tests

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Model/DataStore.h"
#include "../Controller/SampleController.h"
#include "../Controller/OrderController.h"
#include "../Controller/ShippingController.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;

class ShippingControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DataStore::instance().resetForTest();
        sampleCtrl_.registerSample("S001", "Silicon A", 10, 0.9f);
        // Set enough stock so approve() -> Confirmed by default
        auto sampleOpt = DataStore::instance().findSample("S001");
        if (sampleOpt.has_value()) {
            Sample s = *sampleOpt;
            s.stock = 100;
            DataStore::instance().updateSample(s);
        }
    }
    void TearDown() override {
        DataStore::instance().resetForTest();
    }

    // Helper: create an order and approve it -> Confirmed
    std::string makeConfirmedOrder(int qty = 5) {
        std::string id = orderCtrl_.createOrder("Customer", "S001", qty);
        orderCtrl_.approveOrder(id);
        return id;
    }

    SampleController   sampleCtrl_;
    OrderController    orderCtrl_;
    ShippingController shippingCtrl_;
};

// -----------------------------------------------------------------------
// getConfirmedOrders
// -----------------------------------------------------------------------

TEST_F(ShippingControllerTest, GetConfirmedOrders_ReturnsOnlyConfirmed) {
    makeConfirmedOrder();
    makeConfirmedOrder();

    auto confirmed = shippingCtrl_.getConfirmedOrders();
    ASSERT_THAT(confirmed, SizeIs(2));
    for (const auto& o : confirmed) {
        EXPECT_EQ(o.status, OrderStatus::Confirmed);
    }
}

TEST_F(ShippingControllerTest, GetConfirmedOrders_ExcludesReserved) {
    orderCtrl_.createOrder("Customer", "S001", 3);

    auto confirmed = shippingCtrl_.getConfirmedOrders();
    EXPECT_THAT(confirmed, IsEmpty());
}

TEST_F(ShippingControllerTest, GetConfirmedOrders_ExcludesReleased) {
    std::string id = makeConfirmedOrder();
    shippingCtrl_.processShipping(id);

    auto confirmed = shippingCtrl_.getConfirmedOrders();
    EXPECT_THAT(confirmed, IsEmpty());
}

// -----------------------------------------------------------------------
// processShipping
// -----------------------------------------------------------------------

TEST_F(ShippingControllerTest, ProcessShipping_Confirmed_ReturnsTrue) {
    std::string id = makeConfirmedOrder();
    bool ok = shippingCtrl_.processShipping(id);
    EXPECT_TRUE(ok);
}

TEST_F(ShippingControllerTest, ProcessShipping_Confirmed_StatusBecomesRelease) {
    std::string id = makeConfirmedOrder();
    shippingCtrl_.processShipping(id);

    auto order = DataStore::instance().findOrder(id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->status, OrderStatus::Release);
}

TEST_F(ShippingControllerTest, ProcessShipping_Reserved_ReturnsFalse) {
    std::string id = orderCtrl_.createOrder("Customer", "S001", 3);
    bool ok = shippingCtrl_.processShipping(id);
    EXPECT_FALSE(ok);
}

TEST_F(ShippingControllerTest, ProcessShipping_Producing_ReturnsFalse) {
    // Reset stock to 0 so approve -> Producing
    auto sampleOpt = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sampleOpt.has_value());
    Sample s = *sampleOpt;
    s.stock = 0;
    DataStore::instance().updateSample(s);

    std::string id = orderCtrl_.createOrder("Customer", "S001", 5);
    orderCtrl_.approveOrder(id);

    bool ok = shippingCtrl_.processShipping(id);
    EXPECT_FALSE(ok);
}

TEST_F(ShippingControllerTest, ProcessShipping_InvalidId_ReturnsFalse) {
    bool ok = shippingCtrl_.processShipping("NON-EXISTENT-ORDER");
    EXPECT_FALSE(ok);
}

TEST_F(ShippingControllerTest, ProcessShipping_AfterRelease_OrderNotInConfirmedList) {
    std::string id = makeConfirmedOrder();
    shippingCtrl_.processShipping(id);

    auto confirmed = shippingCtrl_.getConfirmedOrders();
    EXPECT_THAT(confirmed, IsEmpty());
}

TEST_F(ShippingControllerTest, ProcessShipping_MultipleOrders_OnlyProcessTarget) {
    std::string id1 = makeConfirmedOrder(3);
    std::string id2 = makeConfirmedOrder(2);

    shippingCtrl_.processShipping(id1);

    auto remaining = shippingCtrl_.getConfirmedOrders();
    ASSERT_THAT(remaining, SizeIs(1));
    EXPECT_EQ(remaining[0].orderId, id2);
}
