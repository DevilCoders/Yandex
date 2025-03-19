#include <kernel/common_server/library/metasearch/helpers/rearrange.h>

#include <library/cpp/testing/unittest/registar.h>
#include <search/web/util/config_parser/config_parser.h>

Y_UNIT_TEST_SUITE(RearrangeHelper) {
    NSc::TValue GetScheme(const TString& options, const TString& rule) {
        NRearr::TSimpleConfiguration rulesConfig = NRearr::ParseSimpleOptions(options, NRearr::TDebugInfo("RearrangeHelperTest",
                                                                                                          NRearr::ThrowError));
        for (const auto& config : rulesConfig.GetRules()) {
            if (config.GetName() != rule) {
                continue;
            }
            return config.GetScheme();
        }
        ythrow yexception() << "no rule " << rule;
    }

    Y_UNIT_TEST(Patch) {
        TString original = "RTYRedirect(CountGroups=1, ReplaceUrl=1)";
        TString patched = original;
        {
            NUtil::TRearrangeOptionsHelper patcher(patched);
            patcher.RuleScheme("RTYRedirect")["url"] = "clck.yandex.ru";
        }

        const NSc::TValue& scheme = GetScheme(patched, "RTYRedirect");
        UNIT_ASSERT_EQUAL(scheme["url"].GetString(), "clck.yandex.ru");
        UNIT_ASSERT_EQUAL(scheme["CountGroups"].GetIntNumber(), 1);
    }

    Y_UNIT_TEST(Simple) {
        TString original = "Fusion(\"d:fresh\"/Age=259200,\"d:fresh_exp\"/Age=259200,\"d:mango\"/Skip=1,FusionMeta=1)";
        TString patched = original;
        {
            NUtil::TRearrangeOptionsHelper patcher(patched);
            /* no patch */
        }

        const NSc::TValue& scheme = GetScheme(patched, "Fusion");
        UNIT_ASSERT_EQUAL(scheme["d:fresh_exp"]["Age"].GetIntNumber(), 259200);
        UNIT_ASSERT(scheme["d:mango"]["Skip"].IsTrue());
        UNIT_ASSERT(scheme["FusionMeta"].IsTrue());
    }
}
