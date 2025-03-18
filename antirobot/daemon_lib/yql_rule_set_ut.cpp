#include "yql_rule_set.h"

#include <library/cpp/testing/unittest/registar.h>


namespace NAntiRobot {


Y_UNIT_TEST_SUITE(TTestYqlRuleSet) {
    Y_UNIT_TEST(MatchesOnce) {
        TProcessorLinearizedFactors factors(TAllFactors::AllFactorsCount());
        factors[1] = 42;

        TYqlRuleSet rules({
            "FALSE",
            TAllFactors::GetFactorNameByIndex(1) + " = 42",
            "2 + 2 == 5",
        });

        const auto matches = rules.Match(factors);
        UNIT_ASSERT_VALUES_EQUAL(matches, TVector<ui32>{1});
    }

    Y_UNIT_TEST(MatchesTwice) {
        TProcessorLinearizedFactors factors(TAllFactors::AllFactorsCount());
        factors[1] = 42;
        factors[3] = 777;

        TYqlRuleSet rules({
            "FALSE",
            TAllFactors::GetFactorNameByIndex(1) + " = 42",
            TAllFactors::GetFactorNameByIndex(3) + " / 7 == 111",
            "2 + 2 == 5",
        });

        const auto matches = rules.Match(factors);
        UNIT_ASSERT_VALUES_EQUAL(matches, (TVector<ui32>{1, 2}));
    }

    Y_UNIT_TEST(MatchesNever) {
        TProcessorLinearizedFactors factors(TAllFactors::AllFactorsCount());
        factors[1] = 100;
        factors[3] = 200;

        TYqlRuleSet rules({
            "FALSE",
            TAllFactors::GetFactorNameByIndex(1) + " = 42",
            TAllFactors::GetFactorNameByIndex(3) + " / 7 == 111",
            "2 + 2 == 5",
        });

        const auto matches = rules.Match(factors);
        UNIT_ASSERT_VALUES_EQUAL(matches, TVector<ui32>{});
    }
}


}
