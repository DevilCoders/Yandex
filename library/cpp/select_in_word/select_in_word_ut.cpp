#include <library/cpp/testing/unittest/gtest.h>

#include "select_in_word.h"

#include <util/generic/xrange.h>
#include <util/random/fast.h>

ui64 SelectInWordSlow(ui64 x, int k) {
    for (const size_t i: xrange(64)) {
        if (x & (1ull << i)) {
            if (!k) {
                return i;
            }
            --k;
        }
    }

    return 64;
}

void Test(ui64 (*Selector)(ui64, int)) {
    EXPECT_EQ(Selector(1, 0), 0);
    EXPECT_EQ(Selector(7, 2), 2);
    EXPECT_EQ(Selector(7ull << 20, 2), 22);

    TFastRng32 rng(889, 0);
    for (const size_t i: xrange(1000000)) {
        Y_UNUSED(i);
        const ui64 x = rng.GenRand64();
        const ui64 k = rng.Uniform(64);
        EXPECT_EQ(Selector(x, k), SelectInWordSlow(x, k));
    }
}

TEST(SelectInWord, X86) {
    Test(&SelectInWordX86);
}

TEST(SelectInWord, Bmi2) {
    Test(&SelectInWordBmi2);
}

TEST(SelectInWord, Dispatch) {
    Test(&SelectInWord);
}
