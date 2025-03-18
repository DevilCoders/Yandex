#include "param_mapping_detect.h"

#include <tools/clustermaster/common/make_vector.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(NDetectParamMapping) {
    Y_UNIT_TEST(Test11NoMatch) {

        TParamListManager listManager;

        TTargetTypeParameters dependent("first", &listManager, 1,
                MakeVector(MakeVector<TString>("aaa")));
        TTargetTypeParameters my("second", &listManager, 1,
                MakeVector(MakeVector<TString>("aaa"), MakeVector<TString>("bbb")));

        const TParamMappings& mappings = DetectMappings(dependent, my);

        UNIT_ASSERT_VALUES_EQUAL("*", mappings.ToString());
    }

    Y_UNIT_TEST(Test11Match) {

        TParamListManager listManager;

        TTargetTypeParameters dependent("first", &listManager, 1,
                MakeVector(MakeVector<TString>("aaa"), MakeVector<TString>("bbb")));
        TTargetTypeParameters my("second", &listManager, 1,
                MakeVector(MakeVector<TString>("aaa"), MakeVector<TString>("bbb")));

        const TParamMappings& mappings = DetectMappings(dependent, my);

        UNIT_ASSERT_VALUES_EQUAL("[0->0]", mappings.ToString());
    }

    Y_UNIT_TEST(Test22Match1) {
        TParamListManager listManager;

        TTargetTypeParameters dependent("first", &listManager, 2,
                MakeVector(
                        MakeVector<TString>("aaa", "000"),
                        MakeVector<TString>("aaa", "111"),
                        MakeVector<TString>("bbb", "111")));
        TTargetTypeParameters my("second", &listManager, 2,
                MakeVector(
                        MakeVector<TString>("000", "xxx"),
                        MakeVector<TString>("111", "zzz"),
                        MakeVector<TString>("111", "www")));

        const TParamMappings& mappings = DetectMappings(dependent, my);

        UNIT_ASSERT_VALUES_EQUAL("[1->0]", mappings.ToString());
    }

    Y_UNIT_TEST(Test32Match2) {
        TParamListManager listManager;

        TTargetTypeParameters dependent("first", &listManager, 3,
                MakeVector(
                        MakeVector<TString>("aaa", "000", "xxx"),
                        MakeVector<TString>("aaa", "111", "www"),
                        MakeVector<TString>("bbb", "111", "zzz")));
        TTargetTypeParameters my("second", &listManager, 2,
                MakeVector(
                        MakeVector<TString>("aaa", "xxx"),
                        MakeVector<TString>("bbb", "zzz"),
                        MakeVector<TString>("bbb", "www")));

        const TParamMappings& mappings = DetectMappings(dependent, my);

        UNIT_ASSERT_VALUES_EQUAL("[0->0,2->1]", mappings.ToString());

    }
}
