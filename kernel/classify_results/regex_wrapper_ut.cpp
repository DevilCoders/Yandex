#include <library/cpp/binsaver/bin_saver.h>
#include <library/cpp/testing/unittest/registar.h>

#include "regex_wrapper.h"

Y_UNIT_TEST_SUITE(RegexWrapperTest) {
    Y_UNIT_TEST(DefIsFalse) {
        TRxMatcher rm;
        UNIT_ASSERT(!rm.IsMatch(""));
    }
    Y_UNIT_TEST(AllIsTrue) {
        TRxMatcher rm(".*");
        UNIT_ASSERT(rm.IsMatch(""));
        UNIT_ASSERT(rm.IsMatch("й"));
        UNIT_ASSERT(rm.IsMatch("йsd"));
    }
    Y_UNIT_TEST(PartialIsTrue) {
        TRxMatcher rm("й");
        UNIT_ASSERT(rm.IsMatch("йsd"));
        UNIT_ASSERT(rm.IsMatch("sйd"));
        UNIT_ASSERT(rm.IsMatch("sdй"));
    }
    Y_UNIT_TEST(FalseCases) {
        TRxMatcher rm("й\\d");
        UNIT_ASSERT(!rm.IsMatch("йй"));
        UNIT_ASSERT(!rm.IsMatch("1й"));
    }
    Y_UNIT_TEST(TrueCases) {
        TRxMatcher rm("\\w\\d");
        UNIT_ASSERT(rm.IsMatch("й0"));
        UNIT_ASSERT(rm.IsMatch("z9"));
    }
}
