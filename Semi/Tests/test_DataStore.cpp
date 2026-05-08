// test_DataStore.cpp
// DataStore Model layer unit tests
// gmock/gtest based (SEMI_TEST macro disables file I/O)

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include "../Model/DataStore.h"
#include "../Model/Order.h"

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

// -----------------------------------------------------------------------
// DataStore::save() - covers function entry (SEMI_TEST mode makes it no-op)
// -----------------------------------------------------------------------

TEST_F(DataStoreTest, Save_DoesNotThrow) {
    // In SEMI_TEST mode save() is a no-op, but the function entry is covered
    EXPECT_NO_THROW(DataStore::instance().save());
}

TEST_F(DataStoreTest, Save_WithData_DoesNotThrow) {
    Sample s;
    s.id = "S_SAVE"; s.name = "Save Test";
    s.avgProductionTimeSec = 5; s.yield = 0.9f; s.stock = 3;
    DataStore::instance().addSample(s);

    Order o;
    o.orderId = "ORD-SAVE"; o.customer = "SaveCust"; o.sampleId = "S_SAVE";
    o.quantity = 1; o.status = OrderStatus::Reserved;
    o.createdAt = o.updatedAt = std::time(nullptr);
    DataStore::instance().addOrder(o);

    ProductionJob job;
    job.jobId = "JOB-SAVE"; job.orderId = "ORD-SAVE"; job.sampleId = "S_SAVE";
    job.quantity = 1; job.shortfall = 0; job.startTime = 0; job.durationSec = 10;
    DataStore::instance().addProductionJob(job);

    // Direct call to save() - covers SEMI_TEST early return line
    EXPECT_NO_THROW(DataStore::instance().save());
}

// -----------------------------------------------------------------------
// Order.cpp - orderStatusToString / orderStatusFromString full coverage
// -----------------------------------------------------------------------

TEST(OrderStatusTest, ToStringReserved) {
    EXPECT_EQ(orderStatusToString(OrderStatus::Reserved), "Reserved");
}

TEST(OrderStatusTest, ToStringRejected) {
    EXPECT_EQ(orderStatusToString(OrderStatus::Rejected), "Rejected");
}

TEST(OrderStatusTest, ToStringProducing) {
    EXPECT_EQ(orderStatusToString(OrderStatus::Producing), "Producing");
}

TEST(OrderStatusTest, ToStringConfirmed) {
    EXPECT_EQ(orderStatusToString(OrderStatus::Confirmed), "Confirmed");
}

TEST(OrderStatusTest, ToStringRelease) {
    EXPECT_EQ(orderStatusToString(OrderStatus::Release), "Release");
}

TEST(OrderStatusTest, ToStringDefault_UnknownEnum) {
    // Cover default branch: cast invalid integer to OrderStatus
    EXPECT_EQ(orderStatusToString(static_cast<OrderStatus>(999)), "Unknown");
}

TEST(OrderStatusTest, FromStringReserved) {
    EXPECT_EQ(orderStatusFromString("Reserved"), OrderStatus::Reserved);
}

TEST(OrderStatusTest, FromStringRejected) {
    EXPECT_EQ(orderStatusFromString("Rejected"), OrderStatus::Rejected);
}

TEST(OrderStatusTest, FromStringProducing) {
    EXPECT_EQ(orderStatusFromString("Producing"), OrderStatus::Producing);
}

TEST(OrderStatusTest, FromStringConfirmed) {
    EXPECT_EQ(orderStatusFromString("Confirmed"), OrderStatus::Confirmed);
}

TEST(OrderStatusTest, FromStringRelease) {
    EXPECT_EQ(orderStatusFromString("Release"), OrderStatus::Release);
}

TEST(OrderStatusTest, FromStringUnknown_FallbackReserved) {
    // Unknown string falls back to Reserved
    EXPECT_EQ(orderStatusFromString("UNKNOWN_STATUS"), OrderStatus::Reserved);
}

// -----------------------------------------------------------------------
// DataStore file I/O integration tests (uses setDbPathForTest to enable I/O)
// Covers: dbFilePath(), ensureDbDir(), load(), saveUnlocked() actual logic
// -----------------------------------------------------------------------

class DataStoreFileIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temp file so we don't pollute the real DB
        testPath_ = std::filesystem::temp_directory_path().string() + "\\semi_test_data.json";
        // Remove any leftover file from previous run
        std::filesystem::remove(testPath_);
        DataStore::instance().resetForTest();
        DataStore::instance().setDbPathForTest(testPath_);
    }
    void TearDown() override {
        DataStore::instance().resetForTest();
        std::filesystem::remove(testPath_);
        std::filesystem::remove(testPath_ + ".tmp");
    }

    std::string testPath_;
};

TEST_F(DataStoreFileIOTest, SaveAndLoad_SampleRoundtrip) {
    Sample s;
    s.id = "S_IO"; s.name = "IO Test";
    s.avgProductionTimeSec = 15; s.yield = 0.85f; s.stock = 7;
    DataStore::instance().addSample(s);  // internally calls saveUnlocked()

    // Verify the file was actually created
    EXPECT_TRUE(std::filesystem::exists(testPath_));

    // Reset in-memory state, re-set path, then reload from file
    DataStore::instance().resetForTest();
    DataStore::instance().setDbPathForTest(testPath_);

    // load() should parse the previously saved data
    DataStore::instance().load();

    auto samples = DataStore::instance().getSamples();
    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0].id, "S_IO");
    EXPECT_EQ(samples[0].name, "IO Test");
    EXPECT_EQ(samples[0].stock, 7);
}

TEST_F(DataStoreFileIOTest, SaveAndLoad_OrderRoundtrip) {
    // First add a sample so order references valid sampleId
    Sample s;
    s.id = "S_IO2"; s.name = "IO Sample2";
    s.avgProductionTimeSec = 5; s.yield = 0.9f; s.stock = 0;
    DataStore::instance().addSample(s);

    Order o;
    o.orderId = "ORD-IO"; o.customer = "IOCust"; o.sampleId = "S_IO2";
    o.quantity = 3; o.status = OrderStatus::Confirmed;
    o.createdAt = o.updatedAt = std::time(nullptr);
    DataStore::instance().addOrder(o);

    DataStore::instance().resetForTest();
    DataStore::instance().setDbPathForTest(testPath_);
    DataStore::instance().load();

    auto orders = DataStore::instance().getOrders();
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0].orderId, "ORD-IO");
    EXPECT_EQ(orders[0].status, OrderStatus::Confirmed);
}

TEST_F(DataStoreFileIOTest, SaveAndLoad_ProductionJobRoundtrip) {
    ProductionJob job;
    job.jobId = "JOB-IO"; job.orderId = "ORD-IO"; job.sampleId = "S_IO";
    job.quantity = 10; job.shortfall = 2;
    job.startTime = std::time(nullptr); job.durationSec = 60;
    DataStore::instance().addProductionJob(job);

    DataStore::instance().resetForTest();
    DataStore::instance().setDbPathForTest(testPath_);
    DataStore::instance().load();

    auto jobs = DataStore::instance().getProductionJobs();
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_EQ(jobs[0].jobId, "JOB-IO");
    EXPECT_EQ(jobs[0].shortfall, 2);
}

TEST_F(DataStoreFileIOTest, Load_NonExistentFile_NoThrow) {
    // load() on non-existent file should silently return
    std::string nonExistent = std::filesystem::temp_directory_path().string() + "\\semi_nonexist.json";
    std::filesystem::remove(nonExistent);
    DataStore::instance().resetForTest();
    DataStore::instance().setDbPathForTest(nonExistent);
    EXPECT_NO_THROW(DataStore::instance().load());
    EXPECT_TRUE(DataStore::instance().getSamples().empty());
}

TEST_F(DataStoreFileIOTest, Load_InvalidJsonFile_NoThrow) {
    // load() on a file with invalid JSON should silently discard
    {
        std::ofstream f(testPath_);
        f << "{ this is not valid json !!!";
    }
    EXPECT_NO_THROW(DataStore::instance().load());
    EXPECT_TRUE(DataStore::instance().getSamples().empty());
}

TEST_F(DataStoreFileIOTest, Save_DirectCall_WritesFile) {
    Sample s;
    s.id = "S_DIRECT"; s.name = "Direct Save";
    s.avgProductionTimeSec = 1; s.yield = 1.0f; s.stock = 0;
    DataStore::instance().addSample(s);

    // Explicitly call save() (which acquires mutex and calls saveUnlocked)
    EXPECT_NO_THROW(DataStore::instance().save());
    EXPECT_TRUE(std::filesystem::exists(testPath_));
}

// -----------------------------------------------------------------------
// dbFilePath() real path computation (covers GetModuleFileNameA branch)
// When testDbPath_ is empty, dbFilePath() computes path from exe location
// -----------------------------------------------------------------------

TEST(DataStoreDbFilePathTest, GetDbFilePathForTest_ReturnsNonEmptyString) {
    // Call getDbFilePathForTest() with no testDbPath_ set -> invokes GetModuleFileNameA branch
    DataStore::instance().resetForTest();
    std::string path = DataStore::instance().getDbFilePathForTest();
    EXPECT_FALSE(path.empty());
    // The path should contain "DB" and end with "data.json"
    EXPECT_NE(path.find("data.json"), std::string::npos);
}

TEST(DataStoreDbFilePathTest, GetDbFilePathForTest_WithTestPath_ReturnsTestPath) {
    DataStore::instance().resetForTest();
    DataStore::instance().setDbPathForTest("C:\\Temp\\custom_test.json");
    std::string path = DataStore::instance().getDbFilePathForTest();
    EXPECT_EQ(path, "C:\\Temp\\custom_test.json");
    DataStore::instance().resetForTest();
}
