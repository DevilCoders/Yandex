#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestSimple, OK) {
    EXPECT_EQ(1, 1);
}

TEST(TestSimple, Failure) {
    EXPECT_EQ(1, 2);
    EXPECT_EQ(1, 3);
}

TEST(TestSimple, DISABLED_Test) {
    EXPECT_EQ(1, 3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
