#include <library/cpp/testing/unittest/registar.h>

#include "req_less.h"

#include <util/generic/array_size.h>

using namespace NIdxOps;

Y_UNIT_TEST_SUITE(TestReqLess) {
    Y_UNIT_TEST(TestStrLess) {
        std::pair<TRequestId, TRequestId> pairs[] = {
           {0, 0},
           {0, 1},
           {1, 1},
           {2, 12},
           {2, 1999},
           {50, 49},
           {Max<TRequestId>(), Max<TRequestId>()},
           {Max<TRequestId>() - 1, Max<TRequestId>()}
        };
        for (size_t i = 0; i != Y_ARRAY_SIZE(pairs); ++i) {
            UNIT_ASSERT_EQUAL(StrLessSlow(pairs[i].first, pairs[i].second),
                StrLess(pairs[i].first, pairs[i].second));
            UNIT_ASSERT_EQUAL(StrLessSlow(pairs[i].second, pairs[i].first),
                StrLess(pairs[i].second, pairs[i].first));
         }
     }
};
