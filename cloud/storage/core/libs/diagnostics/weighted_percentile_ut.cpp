#include "weighted_percentile.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

void AssertEqual(const TVector<double>& l, const TVector<double>& r, double epsilon)
{
    UNIT_ASSERT_VALUES_EQUAL(l.size(), r.size());
    for (size_t i = 0; i < l.size(); ++i) {
        UNIT_ASSERT_C((fabs(l[i]-r[i]) <= epsilon), "Wrong value");
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TWeightedPercentileTest)
{
    Y_UNIT_TEST(ShouldCalculateWeightedPercentiles)
    {
        {
            TVector<TBucketInfo> buckets {
                {100, 10},
                {200, 40},
                {300, 20} ,
                {400, 30},
                {500,  0}};
            TVector<TPercentileDesc> percentiles {
                { 0.2,  "20"},
                { 0.4,  "40"},
                { 0.5,  "50"},
                { 0.7,  "70"},
                {0.85,  "85"},
                {   1, "100"}};

            auto result = CalculateWeightedPercentiles(buckets, percentiles);
            AssertEqual(result, {125, 175, 200, 300, 350, 400}, 0.01);
        }

        {
            TVector<TBucketInfo> buckets {
                {1, 10},
                {2, 40},
                {3, 20},
                {4, 30},
                {5,  0}};
            TVector<TPercentileDesc> percentiles {
                {   0,  "0"},
                { 0.5, "50"},
                { 0.6, "60"},
                { 0.7, "70"},
                { 0.8, "80"},
                { 0.9, "90"},
                {0.99, "99"}};

            auto result = CalculateWeightedPercentiles(buckets, percentiles);
            AssertEqual(result, {0, 2, 2.5, 3, 3.33, 3.66, 3.97}, 0.01);
        }
    }
}

}   // namespace NCloud
