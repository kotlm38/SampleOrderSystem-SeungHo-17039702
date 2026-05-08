// test_SampleController.cpp
// SampleController unit tests

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../Model/DataStore.h"
#include "../Controller/SampleController.h"

using ::testing::SizeIs;
using ::testing::IsEmpty;

class SampleControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        DataStore::instance().resetForTest();
    }
    void TearDown() override {
        DataStore::instance().resetForTest();
    }

    SampleController ctrl_;
};

// -----------------------------------------------------------------------
// registerSample
// -----------------------------------------------------------------------

TEST_F(SampleControllerTest, Register_Success) {
    bool ok = ctrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    EXPECT_TRUE(ok);

    auto all = ctrl_.getAllSamples();
    ASSERT_THAT(all, SizeIs(1));
    EXPECT_EQ(all[0].id,   "S001");
    EXPECT_EQ(all[0].name, "Silicon A");
}

TEST_F(SampleControllerTest, Register_DuplicateId_ReturnsFalse) {
    ctrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    bool second = ctrl_.registerSample("S001", "Silicon A copy", 5, 0.8f);
    EXPECT_FALSE(second);
    ASSERT_THAT(ctrl_.getAllSamples(), SizeIs(1));
}

TEST_F(SampleControllerTest, GetAll_Empty_WhenNoneRegistered) {
    EXPECT_THAT(ctrl_.getAllSamples(), IsEmpty());
}

TEST_F(SampleControllerTest, InitialStock_IsZero) {
    ctrl_.registerSample("S002", "Chip B", 5, 0.85f);
    auto sample = DataStore::instance().findSample("S002");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(sample->stock, 0);
}

TEST_F(SampleControllerTest, MultipleRegistrations_CountMatches) {
    ctrl_.registerSample("S001", "A", 10, 0.9f);
    ctrl_.registerSample("S002", "B", 20, 0.8f);
    ctrl_.registerSample("S003", "C", 15, 0.85f);
    ASSERT_THAT(ctrl_.getAllSamples(), SizeIs(3));
}

// -----------------------------------------------------------------------
// search
// -----------------------------------------------------------------------

TEST_F(SampleControllerTest, SearchById_Found) {
    ctrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    ctrl_.registerSample("S002", "Chip B",     5, 0.8f);

    auto result = ctrl_.search(SampleSearchField::Id, "S001");
    ASSERT_THAT(result, SizeIs(1));
    EXPECT_EQ(result[0].id, "S001");
}

TEST_F(SampleControllerTest, SearchByName_Found) {
    ctrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    ctrl_.registerSample("S002", "Chip B",     5, 0.8f);

    auto result = ctrl_.search(SampleSearchField::Name, "Silicon");
    ASSERT_THAT(result, SizeIs(1));
    EXPECT_EQ(result[0].name, "Silicon A");
}

TEST_F(SampleControllerTest, SearchByName_NotFound) {
    ctrl_.registerSample("S001", "Silicon A", 10, 0.9f);

    auto result = ctrl_.search(SampleSearchField::Name, "Nonexistent");
    EXPECT_THAT(result, IsEmpty());
}

TEST_F(SampleControllerTest, SearchByName_PartialMatch_MultipleResults) {
    ctrl_.registerSample("S001", "Silicon A", 10, 0.9f);
    ctrl_.registerSample("S002", "Silicon B", 20, 0.8f);
    ctrl_.registerSample("S003", "Chip C",     5, 0.7f);

    auto result = ctrl_.search(SampleSearchField::Name, "Silicon");
    ASSERT_THAT(result, SizeIs(2));
}

TEST_F(SampleControllerTest, SearchById_PartialMatch) {
    ctrl_.registerSample("S001", "A", 10, 0.9f);
    ctrl_.registerSample("S002", "B", 10, 0.8f);

    auto result = ctrl_.search(SampleSearchField::Id, "S00");
    ASSERT_THAT(result, SizeIs(2));
}
