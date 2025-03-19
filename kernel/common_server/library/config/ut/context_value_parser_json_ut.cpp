#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

#include <kernel/common_server/library/config/context_value_parser_json.h>

using TSampleContextValue = NConfig::TContextValue<int>;

inline TSampleContextValue MakeSampleContextValueDraft() {
    return TSampleContextValue(&std::min, 10);
}

inline TMaybe<TSampleContextValue> TryMakeSampleContextValue(const TString& jsonText, const TMap<TString, TString>& implicitContext = {}) {
    TMaybe<TSampleContextValue> res = Nothing();
    TStringStream ss(jsonText);
    TSampleContextValue::TRules rules;
    if (!AppendContextValueRulesFromJson(&rules, NJson::ReadJsonTree(&ss, true), implicitContext)) {
        return res;
    }
    res = MakeSampleContextValueDraft();
    res->Init(rules);
    return res;
}

inline TSampleContextValue MakeSampleContextValue(const TString& jsonText, const TMap<TString, TString>& implicitContext = {}) {
    TMaybe<TSampleContextValue> maybeValue = TryMakeSampleContextValue(jsonText, implicitContext);
    UNIT_ASSERT(maybeValue);
    return *maybeValue;
}

Y_UNIT_TEST_SUITE(ContextValueParserJson) {

    Y_UNIT_TEST(ParseEmptyList) {
        auto value = MakeSampleContextValue("[]");

        UNIT_ASSERT(value.IsInitialized());
        UNIT_ASSERT_EQUAL(0, value.GetRules().size());
    }

    Y_UNIT_TEST(ParseDefaultRule) {
        auto value = MakeSampleContextValue(R"([{"value":42}])");

        UNIT_ASSERT(value.IsInitialized());

        UNIT_ASSERT_EQUAL(1, value.GetRules().size());

        UNIT_ASSERT_EQUAL(42, value.GetRules()[0].Value);
        UNIT_ASSERT_EQUAL(0, value.GetRules()[0].ContextChecks.size());
        UNIT_ASSERT_EQUAL(0, value.GetRules()[0].Priority);
    }

    Y_UNIT_TEST(ParseTwoRules) {
        auto value = MakeSampleContextValue(R"([{"value":42, "priority":50},{"value":20,"for":{"employer":"abc", "region":"msk"}}])");

        UNIT_ASSERT(value.IsInitialized());

        UNIT_ASSERT_EQUAL(2, value.GetRules().size());

        UNIT_ASSERT_EQUAL(42, value.GetRules()[0].Value);
        UNIT_ASSERT_EQUAL(50, value.GetRules()[0].Priority);
        UNIT_ASSERT_EQUAL(0, value.GetRules()[0].ContextChecks.size());

        UNIT_ASSERT_EQUAL(20, value.GetRules()[1].Value);
        UNIT_ASSERT_EQUAL(200, value.GetRules()[1].Priority);
        UNIT_ASSERT_EQUAL(2, value.GetRules()[1].ContextChecks.size());
        UNIT_ASSERT_EQUAL("abc", *value.GetRules()[1].ContextChecks.FindPtr("employer"));
        UNIT_ASSERT_EQUAL("msk", *value.GetRules()[1].ContextChecks.FindPtr("region"));
    }

    Y_UNIT_TEST(AcceptImplicitContextDuplication) {
        auto value = MakeSampleContextValue(R"([{"value":42, "priority":50},{"value":20,"for":{"employer":"abc", "region":"msk"}}])", {{"employer", "abc"}});

        UNIT_ASSERT(value.IsInitialized());

        UNIT_ASSERT_EQUAL(2, value.GetRules().size());

        UNIT_ASSERT_EQUAL(42, value.GetRules()[0].Value);
        UNIT_ASSERT_EQUAL(50, value.GetRules()[0].Priority);
        UNIT_ASSERT_EQUAL(1, value.GetRules()[0].ContextChecks.size());
        UNIT_ASSERT_EQUAL("abc", *value.GetRules()[0].ContextChecks.FindPtr("employer"));

        UNIT_ASSERT_EQUAL(20, value.GetRules()[1].Value);
        UNIT_ASSERT_EQUAL(200, value.GetRules()[1].Priority);
        UNIT_ASSERT_EQUAL(2, value.GetRules()[1].ContextChecks.size());
        UNIT_ASSERT_EQUAL("abc", *value.GetRules()[1].ContextChecks.FindPtr("employer"));
        UNIT_ASSERT_EQUAL("msk", *value.GetRules()[1].ContextChecks.FindPtr("region"));
    }

    Y_UNIT_TEST(RejectImplicitContextOverride) {
        auto value = MakeSampleContextValue(R"([{"value":42, "priority":50},{"value":20,"for":{"employer":"abc", "region":"msk"}}])", {{"employer", "xyz"}});

        UNIT_ASSERT(value.IsInitialized());

        UNIT_ASSERT_EQUAL(1, value.GetRules().size());

        UNIT_ASSERT_EQUAL(42, value.GetRules()[0].Value);
        UNIT_ASSERT_EQUAL(50, value.GetRules()[0].Priority);
        UNIT_ASSERT_EQUAL(1, value.GetRules()[0].ContextChecks.size());
        UNIT_ASSERT_EQUAL("xyz", *value.GetRules()[0].ContextChecks.FindPtr("employer"));
    }
}
