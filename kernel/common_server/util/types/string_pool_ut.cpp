#include "string_pool.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>

Y_UNIT_TEST_SUITE(StringPoolSuite) {
    Y_UNIT_TEST(Simple) {
        TStringPool sp;
        sp.Get("a");
        sp.Get("c");
        sp.Get("b");
        sp.Get("b");
        sp.Get("_a");
        sp.Get("_a");
        sp.Get("_a");
        sp.Get("_c");
        sp.Get("_c");
        sp.Get("b");
        sp.Get("_b");
        sp.Get("_b");
        sp.Get("b");
        UNIT_ASSERT_VALUES_EQUAL(sp.GetDebugString(), "_a _b _c a b c");
        UNIT_ASSERT(sp.Has("a"));
        UNIT_ASSERT(sp.Has("b"));
        UNIT_ASSERT(sp.Has("c"));
        UNIT_ASSERT(sp.Has("_a"));
        UNIT_ASSERT(sp.Has("_b"));
        UNIT_ASSERT(sp.Has("_c"));
        UNIT_ASSERT(!sp.Has("d"));
        UNIT_ASSERT_VALUES_EQUAL(sp.GetDebugString(), "_a _b _c a b c");
        sp.Get("d");
        UNIT_ASSERT_VALUES_EQUAL(sp.GetDebugString(), "_a _b _c a b c d");
        UNIT_ASSERT(sp.Has("d"));
    }
}
