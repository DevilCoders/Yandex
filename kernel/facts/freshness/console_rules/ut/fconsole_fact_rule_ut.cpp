#include <library/cpp/testing/unittest/registar.h>

#include <search/session/searcherprops.h>
#include <search/idl/meta.pb.h>

#include <kernel/facts/freshness/console_rules/fconsole_fact_rule.h>

#include <util/generic/algorithm.h>

Y_UNIT_TEST_SUITE(TFConsoleFactRule) {
    Y_UNIT_TEST(RuleSerialization) {
        TVector<NFacts::TFConsoleFactRule> srcRules = {
            {"ban_ban", {"field1", "field2"}, 1234567890},
            {"ban_fact", {"field1", "field2", "field3"}, 1324567890},
            {"ban_fact_by_url", {"field0"}, 1324567809},
            {"ban_ban", {"field1", "field2"}, 1234567890},
        };
        TVector<TVector<TString>> srcRulesSerializedExpected = {
            {"ban_ban", "field1", "field2"},
            {"ban_fact", "field1", "field2", "field3"},
            {"ban_fact_by_url", "field0"},
            {"ban_ban", "field1", "field2"},
        };

        NMetaProtocol::TReport dstReport;
        NFacts::SerializeFConsoleRulesToReport(srcRules, dstReport);

        TSearcherProps props;
        for (const auto& searcherProp : dstReport.GetSearcherProp()) {
            props.Set(searcherProp.GetKey(), searcherProp.GetValue());
        }

        Sort(srcRulesSerializedExpected);
        auto srcRulesSerialized = NFacts::FetchFConsoleFactRules(props);
        Sort(srcRulesSerialized);
        UNIT_ASSERT_EQUAL(srcRulesSerializedExpected, srcRulesSerialized);
    }
}
