// test_DataStore.cpp
// DataStore Model layer unit tests
// gmock/gtest based (SEMI_TEST macro disables file I/O)

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Model/DataStore.h"

// Fixture: reset DataStore state before each test
class DataStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        DataStore::instance().resetForTest();
    }
    void TearDown() override {
        DataStore::instance().resetForTest();
    }
};

// -----------------------------------------------------------------------
// Sample CRUD
// -----------------------------------------------------------------------

TEST_F(DataStoreTest, AddAndGetSample) {
    Sample s;
    s.id                   = "S001";
    s.name                 = "Silicon Wafer A";
    s.avgProductionTimeSec = 10;
    s.yield                = 0.9f;
    s.stock                = 0;

    DataStore::instance().addSample(s);

    auto samples = DataStore::instance().getSamples();
    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0].id,    "S001");
    EXPECT_EQ(samples[0].name,  "Silicon Wafer A");
    EXPECT_EQ(samples[0].stock, 0);
}

TEST_F(DataStoreTest, FindSample_Hit) {
    Sample s;
    s.id = "S002"; s.name = "Chip B";
    s.avgProductionTimeSec = 5; s.yield = 0.8f; s.stock = 10;
    DataStore::instance().addSample(s);

    auto found = DataStore::instance().findSample("S002");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name,  "Chip B");
    EXPECT_EQ(found->stock, 10);
}

TEST_F(DataStoreTest, FindSample_Miss) {
    auto found = DataStore::instance().findSample("NONEXISTENT");
    EXPECT_FALSE(found.has_value());
}

TEST_F(DataStoreTest, UpdateSample_StockChange) {
    Sample s;
    s.id = "S003"; s.name = "Test Sample";
    s.avgProductionTimeSec = 3; s.yield = 0.95f; s.stock = 5;
    DataStore::instance().addSample(s);

    s.stock = 20;
    DataStore::instance().updateSample(s);

    auto updated = DataStore::instance().findSample("S003");
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->stock, 20);
}

// -----------------------------------------------------------------------
// Order CRUD
// -----------------------------------------------------------------------

TEST_F(DataStoreTest, AddAndGetOrder) {
    Order o;
    o.orderId   = "ORD-001";
    o.customer  = "CustomerA";
    o.sampleId  = "S001";
    o.quantity  = 5;
    o.status    = OrderStatus::Reserved;
    o.createdAt = std::time(nullptr);
    o.updatedAt = o.createdAt;
    DataStore::instance().addOrder(o);

    auto orders = DataStore::instance().getOrders();
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0].orderId,  "ORD-001");
    EXPECT_EQ(orders[0].customer, "CustomerA");
    EXPECT_EQ(orders[0].status,   OrderStatus::Reserved);
}

TEST_F(DataStoreTest, RemoveOrder_ListBecomesEmpty) {
    Order o;
    o.orderId = "ORD-002"; o.customer = "CustomerB"; o.sampleId = "S001";
    o.quantity = 3; o.status = OrderStatus::Reserved;
    o.createdAt = o.updatedAt = std::time(nullptr);
    DataStore::instance().addOrder(o);

    DataStore::instance().removeOrder("ORD-002");

    EXPECT_TRUE(DataStore::instance().getOrders().empty());
}

TEST_F(DataStoreTest, UpdateOrder_StatusChange) {
    Order o;
    o.orderId = "ORD-003"; o.customer = "CustomerC"; o.sampleId = "S001";
    o.quantity = 2; o.status = OrderStatus::Reserved;
    o.createdAt = o.updatedAt = std::time(nullptr);
    DataStore::instance().addOrder(o);

    o.status = OrderStatus::Confirmed;
    DataStore::instance().updateOrder(o);

    auto found = DataStore::instance().findOrder("ORD-003");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->status, OrderStatus::Confirmed);
}

TEST_F(DataStoreTest, FindOrder_Miss) {
    auto found = DataStore::instance().findOrder("NO-ORDER");
    EXPECT_FALSE(found.has_value());
}

// -----------------------------------------------------------------------
// ProductionJob CRUD
// -----------------------------------------------------------------------

TEST_F(DataStoreTest, AddAndRemoveProductionJob) {
    ProductionJob job;
    job.jobId       = "JOB-001";
    job.orderId     = "ORD-001";
    job.sampleId    = "S001";
    job.quantity    = 10;
    job.shortfall   = 5;
    job.startTime   = 0;
    job.durationSec = 30;
    DataStore::instance().addProductionJob(job);

    ASSERT_EQ(DataStore::instance().getProductionJobs().size(), 1u);

    DataStore::instance().removeProductionJob("JOB-001");
    EXPECT_TRUE(DataStore::instance().getProductionJobs().empty());
}

TEST_F(DataStoreTest, UpdateProductionJob_StartTime) {
    ProductionJob job;
    job.jobId = "JOB-002"; job.orderId = "ORD-002"; job.sampleId = "S002";
    job.quantity = 5; job.shortfall = 2; job.startTime = 0; job.durationSec = 20;
    DataStore::instance().addProductionJob(job);

    std::time_t now = std::time(nullptr);
    job.startTime = now;
    DataStore::instance().updateProductionJob(job);

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_EQ(jobs[0].startTime, now);
}
