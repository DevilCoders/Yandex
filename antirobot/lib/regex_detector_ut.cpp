#include "regex_detector.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>


namespace NAntiRobot {


Y_UNIT_TEST_SUITE(RegexDetector) {
    Y_UNIT_TEST(Detect) {
        TRegexDetector detector({"hello", "hel.*", "hellox"});

        auto result = detector.Detect("hello");
        Sort(result);

        UNIT_ASSERT_VALUES_EQUAL(result, (TVector<size_t>{0, 1}));
    }

    Y_UNIT_TEST(DetectMultipleScanners) {
        TRegexDetector detector({"hello", "hel.*", "hellox"}, 1);

        auto result = detector.Detect("hello");
        Sort(result);

        UNIT_ASSERT_GE(detector.NumScanners(), 1);
        UNIT_ASSERT_VALUES_EQUAL(result, (TVector<size_t>{0, 1}));
    }
}


} // namespace NAntiRobot
