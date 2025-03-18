#include <library/cpp/testing/gtest/gtest.h>
#include <library/cpp/testing/gtest_boost_extensions/extensions.h>

TEST(PrettyPrinters, Optional) {
    struct T{};

    EXPECT_EQ(testing::PrintToString(boost::optional<int>{}), "(nullopt)");
    EXPECT_EQ(testing::PrintToString(boost::optional<int>{1}), "(1)");
    EXPECT_THAT(testing::PrintToString(boost::optional<T>{T{}}), testing::MatchesRegex("^\\(1-byte object <[0-9A-F]{2}>\\)$"));
}
