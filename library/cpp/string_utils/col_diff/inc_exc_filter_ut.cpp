#include "inc_exc_filter.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestIncExcFilter) {
    Y_UNIT_TEST(TestExc) {
        TIncExcFilter<int> filter;

        filter.AddExc(12);
        filter.AddExc(128);
        filter.AddExc(-12);

        UNIT_ASSERT(filter.Check(0));
        UNIT_ASSERT(filter.Check(-5));
        UNIT_ASSERT(filter.Check(33198));

        UNIT_ASSERT(!filter.Check(128));
        UNIT_ASSERT(!filter.Check(-12));
    }

    Y_UNIT_TEST(TestInc) {
        THashSet<TString> inc;
        inc.insert("a");
        inc.insert("bdwq da");
        inc.insert("adqdqwd");

        TIncExcFilter<TString> filter(inc);

        UNIT_ASSERT(filter.Check("a"));
        UNIT_ASSERT(filter.Check("adqdqwd"));

        UNIT_ASSERT(!filter.Check(""));
        UNIT_ASSERT(!filter.Check("we 3232 "));
    }

    Y_UNIT_TEST(TestIncExc) {
        TIncExcFilter<int> filter;

        filter.AddInc(1232);
        filter.AddInc(33456);
        filter.AddInc(-67);
        filter.AddInc(-1236);
        filter.AddExc(33456);
        filter.AddExc(-1236);

        UNIT_ASSERT(filter.Check(1232));
        UNIT_ASSERT(filter.Check(-67));

        UNIT_ASSERT(!filter.Check(33456));
        UNIT_ASSERT(!filter.Check(-1236));
        UNIT_ASSERT(!filter.Check(0));
    }
}
