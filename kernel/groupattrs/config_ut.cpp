#include <library/cpp/testing/unittest/registar.h>
#include "config.h"


Y_UNIT_TEST_SUITE(TGroupingConfigSuite) {
    Y_UNIT_TEST(InitFromString) {
        {
            TString attrs = "unique_attr:2:unique mid:2 attr_cc_grp:2:named attr_aa_grp:2 single:1";
            NGroupingAttrs::TConfig conf(NGroupingAttrs::TConfig::Index);
            conf.InitFromStringWithTypes(attrs.data());
            UNIT_ASSERT_EQUAL(attrs, conf.ToString());
        }
        {
            TString attrs = "unique_attr:2:unique mid:2 attr_cc_grp:2:named attr_aa_grp:2 single:1";
            NGroupingAttrs::TConfig conf(NGroupingAttrs::TConfig::Search);
            conf.InitFromStringWithTypes(attrs.data());
            UNIT_ASSERT_EQUAL(attrs, conf.ToString());
        }
        {
            TString attrs = "unique_attr:2:unique mid:2 attr_cc_grp:2:named $docid$:1 $docid$:1 attr_aa_grp:2 single:1 $docid$:1";
            NGroupingAttrs::TConfig conf(NGroupingAttrs::TConfig::Index);
            conf.InitFromStringWithTypes(attrs.data());
            TString attrsRes = "unique_attr:2:unique mid:2 attr_cc_grp:2:named attr_aa_grp:2 single:1";
            UNIT_ASSERT_EQUAL(attrsRes, conf.ToString());
        }
    }
}
