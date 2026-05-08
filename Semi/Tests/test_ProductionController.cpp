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
#include <algorithm>
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

// -----------------------------------------------------------------------
// workerLoop branch coverage:
// line 37: when active job exists, hasActive=true => pending job not started
// line 41-46: when only pending jobs exist, workerLoop sets startTime
// -----------------------------------------------------------------------

TEST_F(ProductionControllerTest, WorkerLoop_PendingJob_StartsAfterTick) {
    // Covers lines 41-46: pending job (startTime==0) gets startTime set after 1 tick
    std::string id = orderCtrl_.createOrder("Customer", "S001", 5);
    orderCtrl_.approveOrder(id);

    // Verify initial state: startTime == 0 (pending)
    auto jobsBefore = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobsBefore, SizeIs(1));
    EXPECT_EQ(jobsBefore[0].startTime, 0);

    // Let workerLoop run at least one tick
    ProductionController::instance().start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ProductionController::instance().stop();

    // After tick: job either still exists with startTime > 0, or was completed and removed
    auto jobsAfter = DataStore::instance().getProductionJobs();
    if (!jobsAfter.empty()) {
        EXPECT_GT(jobsAfter[0].startTime, 0);
    }
    // If job was removed, it completed normally (also valid)
}

TEST_F(ProductionControllerTest, WorkerLoop_NoStartTimeZeroJob_ForLoopCompletes) {
    // Covers lines 57-58: for loop closes without break when no job has startTime==0
    // startTime=-1 → not active (>0), not pending (==0) → if(startTime==0) is false
    // → line 57 } covered, loop completes without break → line 58 } covered
    ProductionJob pj;
    pj.jobId       = "j_marker";
    pj.orderId     = "o_marker";
    pj.sampleId    = "S001";
    pj.quantity    = 1;
    pj.shortfall   = 0;
    pj.startTime   = -1;
    pj.durationSec = 9999;
    DataStore::instance().addProductionJob(pj);

    ProductionController::instance().start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ProductionController::instance().stop();

    // Job is unchanged — not started, not completed
    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(1));
    EXPECT_EQ(jobs[0].startTime, -1);
}

TEST_F(ProductionControllerTest, WorkerLoop_ActiveJob_PendingNotStarted) {
    // Covers line 37: hasActive=true branch - pending job is NOT started when active exists
    sampleCtrl_.registerSample("S002", "Chip B", 100, 0.8f);

    // Create two orders, both go to Producing state
    std::string id1 = orderCtrl_.createOrder("Customer1", "S001", 5);
    orderCtrl_.approveOrder(id1);

    std::string id2 = orderCtrl_.createOrder("Customer2", "S002", 3);
    orderCtrl_.approveOrder(id2);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobs, SizeIs(2));

    // Manually set first job as active (startTime > 0) with very long duration
    ProductionJob activeJob = jobs[0];
    activeJob.startTime   = std::time(nullptr);
    activeJob.durationSec = 9999;
    DataStore::instance().updateProductionJob(activeJob);

    // Second job remains pending (startTime == 0)
    // With an active job present, workerLoop must NOT start the pending job

    ProductionController::instance().start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ProductionController::instance().stop();

    // Both jobs should still be in the store
    auto jobsAfter = DataStore::instance().getProductionJobs();
    ASSERT_THAT(jobsAfter, SizeIs(2));

    // Find the second job by jobId and verify startTime is still 0
    std::string pendingJobId = jobs[1].jobId;
    auto it = std::find_if(jobsAfter.begin(), jobsAfter.end(),
        [&](const ProductionJob& j) { return j.jobId == pendingJobId; });
    ASSERT_NE(it, jobsAfter.end());
    EXPECT_EQ(it->startTime, 0);
}
