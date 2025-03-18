#include <gtest/gtest.h>

TEST(GTest, AsanCrash)
{
    if (false) {
        auto p = new int;
        delete p;
        *p = 0;
    }
}
