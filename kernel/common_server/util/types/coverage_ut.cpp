#include <kernel/common_server/util/types/coverage.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/buffer.h>
#include <util/generic/ymath.h>
#include <util/random/random.h>

#include <iterator>

Y_UNIT_TEST_SUITE(RtyUtilCoverageTest) {
    Y_UNIT_TEST(TestCoverageBuildSimple) {
        NUtil::TCoverage<ui32, TString> cov(0, 65533);
        cov.AddHandler(TInterval<ui32>(0, 13105), "abc0");
        cov.AddHandler(TInterval<ui32>(13106, 26212), "abc0");
        cov.AddHandler(TInterval<ui32>(26213, 39318), "abc0");
        cov.AddHandler(TInterval<ui32>(39319, 52425), "abc0");
        cov.AddHandler(TInterval<ui32>(52426, 65533), "abc000");

        auto at = [&cov](size_t index) {
            UNIT_ASSERT(index < cov.size());
            auto i = cov.begin();
            std::advance(i, index);
            UNIT_ASSERT(i != cov.end());
            return *i;
        };

        UNIT_ASSERT_VALUES_EQUAL(cov.size(), 5);
        UNIT_ASSERT_VALUES_EQUAL(at(1).first, TInterval<ui32>(13106, 26212));
        UNIT_ASSERT_VALUES_EQUAL(at(1).second.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(at(1).second[0], "abc0");

        UNIT_ASSERT_VALUES_EQUAL(at(4).first, TInterval<ui32>(52426, 65533));
        UNIT_ASSERT_VALUES_EQUAL(at(4).second.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(at(4).second[0], "abc000");

        cov.AddHandler(TInterval<ui32>(0, 13105), "abc0");
        cov.AddHandler(TInterval<ui32>(13106, 26211), "abc0");
        cov.AddHandler(TInterval<ui32>(26212, 39317), "abc0");
        cov.AddHandler(TInterval<ui32>(39318, 52423), "abc0");
        cov.AddHandler(TInterval<ui32>(52423, 65533), "abc000");

        UNIT_ASSERT_VALUES_EQUAL(cov.size(), 9);

        UNIT_ASSERT_VALUES_EQUAL(at(1).first, TInterval<ui32>(13106, 26211));
        UNIT_ASSERT_VALUES_EQUAL(at(1).second.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(at(2).first, TInterval<ui32>(26212, 26212));
        UNIT_ASSERT_VALUES_EQUAL(at(2).second.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(at(3).first, TInterval<ui32>(26213, 39317));
        UNIT_ASSERT_VALUES_EQUAL(at(3).second.size(), 2);

        auto lb = cov.lower_bound(42000);
        UNIT_ASSERT(lb != cov.end());
        UNIT_ASSERT_VALUES_EQUAL(lb->first, TInterval<ui32>(39319, 52422));
    }
}
