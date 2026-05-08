// test_ProductionController.cpp
// ProductionController unit tests
//
// ProductionController::onJobComplete() is private, so we test it indirectly:
// set job.startTime to a past value so workerLoop processes it immediately.
// durationSec=1 sample + 3s sleep gives enough margin.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include "../Model/DataStore.h"
#include "../Controller/SampleController.h"
#include "../Controller/OrderController.h"
#include "../Controller/ProductionController.h"

using ::testing::IsEmpty;
using ::testing::SizeIs;

class ProductionControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DataStore::instance().resetForTest();
        sampleCtrl_.registerSample("S001", "Silicon A", 1, 0.9f);
    }
    void TearDown() override {
        ProductionController::instance().stop();
        DataStore::instance().resetForTest();
    }

    SampleController sampleCtrl_;
    OrderController  orderCtrl_;
};

// -----------------------------------------------------------------------
// Job complete -> stock increases
// -----------------------------------------------------------------------

TEST_F(ProductionControllerTest, JobComplete_StockIncreases) {
    std::string id = orderCtrl_.createOrder("Customer", "S001", 5);
    orderCtrl_.approveOrder(id);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));

    // Backdate startTime so worker immediately sees job as done
    ProductionJob job = jobs[0];
    job.startTime = std::time(nullptr) - (job.durationSec + 2);
    DataStore::instance().updateProductionJob(job);

    ProductionController::instance().start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ProductionController::instance().stop();

    auto sample = DataStore::instance().findSample("S001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_GT(sample->stock, 0);
}

// -----------------------------------------------------------------------
// Job complete -> order status becomes Confirmed
// -----------------------------------------------------------------------

TEST_F(ProductionControllerTest, JobComplete_OrderStatusConfirmed) {
    std::string id = orderCtrl_.createOrder("Customer", "S001", 3);
    orderCtrl_.approveOrder(id);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));
    ProductionJob job = jobs[0];
    job.startTime = std::time(nullptr) - (job.durationSec + 2);
    DataStore::instance().updateProductionJob(job);

    ProductionController::instance().start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ProductionController::instance().stop();

    auto order = DataStore::instance().findOrder(id);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->status, OrderStatus::Confirmed);
}

// -----------------------------------------------------------------------
// Job complete -> ProductionJob removed
// -----------------------------------------------------------------------

TEST_F(ProductionControllerTest, JobComplete_JobRemovedFromStore) {
    std::string id = orderCtrl_.createOrder("Customer", "S001", 2);
    orderCtrl_.approveOrder(id);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));
    ProductionJob job = jobs[0];
    job.startTime = std::time(nullptr) - (job.durationSec + 2);
    DataStore::instance().updateProductionJob(job);

    ProductionController::instance().start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ProductionController::instance().stop();

    EXPECT_THAT(DataStore::instance().getProductionJobs(), IsEmpty());
}

// -----------------------------------------------------------------------
// getPendingJobs / getActiveJobs classification
// -----------------------------------------------------------------------

TEST_F(ProductionControllerTest, GetPendingJobs_NewJobIsPending) {
    std::string id = orderCtrl_.createOrder("Customer", "S001", 5);
    orderCtrl_.approveOrder(id);

    auto pending = ProductionController::instance().getPendingJobs();
    ASSERT_THAT(pending, SizeIs(1));
    EXPECT_EQ(pending[0].startTime, 0);
}

TEST_F(ProductionControllerTest, GetActiveJobs_StartedJobIsActive) {
    std::string id = orderCtrl_.createOrder("Customer", "S001", 5);
    orderCtrl_.approveOrder(id);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));
    ProductionJob job = jobs[0];
    job.startTime = std::time(nullptr);
    DataStore::instance().updateProductionJob(job);

    auto active = ProductionController::instance().getActiveJobs();
    ASSERT_THAT(active, SizeIs(1));
    EXPECT_GT(active[0].startTime, 0);
}
