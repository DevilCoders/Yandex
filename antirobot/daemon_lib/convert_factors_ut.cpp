#include "convert_factors.h"
#include "threat_level.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

    using namespace NAntiRobot;

    const size_t NUM_AGGR = 4;

    const char* fnamesV3[] = {
#include <antirobot/daemon_lib/factors_versions/factors_3.inc>
    };
    const size_t fnamesV3Size = Y_ARRAY_SIZE(fnamesV3);


    void FillTestFactors(float* factors, size_t size) {
        float f = 1.0;
        for (size_t i = 0; i < size; i++) {
            factors[i] = f;
            f += 1.0;
        }
    }

    int FindFactor(const char* factorName, const char* factors[], int size) {
        for (int i = 0; i < size; i++)
            if (strcmp(factors[i], factorName) == 0)
                return i;
        return -1;
    }
}

namespace NAntiRobot {

    Y_UNIT_TEST_SUITE(TTestConvertFactors) {

        Y_UNIT_TEST(TestFindFactor) {
            const char* names[] = {
                "ab",
                "abc",
                "abcd"
            };

            UNIT_ASSERT_EQUAL(FindFactor("ab", names, Y_ARRAY_SIZE(names)), 0);
            UNIT_ASSERT_EQUAL(FindFactor("abcd", names, Y_ARRAY_SIZE(names)), 2);
            UNIT_ASSERT_EQUAL(FindFactor("abcde", names, Y_ARRAY_SIZE(names)), -1);
        }

        Y_UNIT_TEST(TestConvert) {
            InitFactorsConvert();

            float factorsV3[fnamesV3Size / NUM_AGGR];
            FillTestFactors(factorsV3, Y_ARRAY_SIZE(factorsV3));

            TFactorsWithAggregation values;
            memset(&values, 0, sizeof(values));
            ConvertFactors(3, factorsV3, values);

            for (size_t i = 0; i < TFactorsWithAggregation::RawFactorsWithAggregationCount(); i++) {
                TString factorName = TFactorsWithAggregation::GetFactorNameWithAggregationSuffix(i);
                const char* curFactorName = factorName.c_str();
                int v3Index = FindFactor(curFactorName, fnamesV3, fnamesV3Size);
                if (v3Index != -1) {
                    UNIT_ASSERT_EQUAL(values.GetFactor(i), factorsV3[v3Index]);
                } else {
                    UNIT_ASSERT_DOUBLES_EQUAL(values.GetFactor(i), 0.0, 1e-6);
                }
            }
        }
    }
}

