#include <library/cpp/deprecated/prog_options/prog_options.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TProgOptionsTest) {
    const TVector<TString> parts = {"One","Two","Three","Four"};

    Y_UNIT_TEST(TestSetParts_WhiteList) {
        TOptRes opt {true, "One,Four"};
        THashSet<TString> result;
        SetParts(parts, opt, result);

        UNIT_ASSERT_VALUES_EQUAL(result.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(result.contains("One") && result.contains("Four"), true);
    }

    Y_UNIT_TEST(TestSetParts_BlackList) {
        TOptRes opt {true, "Three"};
        THashSet<TString> result;
        SetParts(parts, opt, true, result);

        UNIT_ASSERT_VALUES_EQUAL(result.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(!result.contains("Three"), true);
    }

    Y_UNIT_TEST(TestSetParts_Nothing) {
        TOptRes opt {false, ""};
        THashSet<TString> result;
        SetParts(parts, opt,result);

        UNIT_ASSERT_VALUES_EQUAL(result.size(), 4);
    }
}
