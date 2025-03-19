#include <library/cpp/testing/unittest/registar.h>

#include <kernel/common_server/library/config/context_value.h>

using TSampleContextValue = NConfig::TContextValue<int>;

inline auto MakeSampleContextValueDraft() {
    return TSampleContextValue(&std::min, 10);
}

inline auto MakeSampleContextValue(TSampleContextValue::TRules rules) {
    auto res = MakeSampleContextValueDraft();
    res.Init(std::move(rules));
    return res;
}

Y_UNIT_TEST_SUITE(ContextValue) {

    Y_UNIT_TEST(DefaultIfAggregateIsEmpty) {
        TSampleContextValue contextValue({}, 10);
        contextValue.Init({});
        UNIT_ASSERT_EQUAL(10, contextValue.Resolve({}));
    }

    Y_UNIT_TEST(DefaultIfNotInitialized) {
        UNIT_ASSERT_EQUAL(10, MakeSampleContextValueDraft().Resolve({}));
    }

    Y_UNIT_TEST(DefaultIfContextUnknown) {
        UNIT_ASSERT_EQUAL(10, MakeSampleContextValue({{{{"employer", "xyz"}}, 20, 0}}).Resolve({{{"region", "msk"}}}));
    }

    Y_UNIT_TEST(InitDefault) {
        auto value = MakeSampleContextValueDraft();
        value.Init({});
        UNIT_ASSERT_EQUAL(10, value.Resolve({}));
    }

    Y_UNIT_TEST(OneSimpleRule) {
        auto value = MakeSampleContextValue({{{{"employer", "abc"}}, 20, 0}});

        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"unrelated", "value"}, {"employer", "abc"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"unrelated", "value"}, {"employer", "xyz"}}}));

        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"employer", "abc"}}, {{"employer", "xyz"}}}));
    }

    Y_UNIT_TEST(OneComplexRule) {
        auto value = MakeSampleContextValue({{{{"employer", "abc"}, {"region", "msk"}}, 20, 0}});

        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"unrelated", "value"}, {"employer", "abc"}, {"region", "msk"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"unrelated", "value"}, {"employer", "xyz"}, {"region", "msk"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"unrelated", "value"}, {"employer", "abc"}, {"region", "spb"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"unrelated", "value"}, {"employer", "xyz"}, {"region", "spb"}}}));

        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"employer", "abc"}, {"region", "msk"}}, {{"employer", "xyz"}, {"region", "spb"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"employer", "xyz"}, {"region", "msk"}}, {{"employer", "abc"}, {"region", "spb"}}}));
    }

    Y_UNIT_TEST(TwoSimpleRules) {
        auto value = MakeSampleContextValue({
                {{{"employer", "abc"}}, 20, 0},
                {{{"region", "msk"}}, 30, 0}
        });

        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"employer", "abc"}, {"region", "msk"}}}));
        UNIT_ASSERT_EQUAL(30, value.Resolve({{{"employer", "xyz"}, {"region", "msk"}}}));
        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"employer", "abc"}, {"region", "spb"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"employer", "xyz"}, {"region", "spb"}}}));
    }

    Y_UNIT_TEST(MixedRules) {
        auto value = MakeSampleContextValue({
                {{{"employer", "abc"}}, 20, 0},
                {{{"region", "msk"}}, 30, 0},
                {{{"employer", "abc"}, {"region", "msk"}}, 40, 100}
        });

        UNIT_ASSERT_EQUAL(40, value.Resolve({{{"employer", "abc"}, {"region", "msk"}}}));
        UNIT_ASSERT_EQUAL(30, value.Resolve({{{"employer", "xyz"}, {"region", "msk"}}}));
        UNIT_ASSERT_EQUAL(20, value.Resolve({{{"employer", "abc"}, {"region", "spb"}}}));
        UNIT_ASSERT_EQUAL(10, value.Resolve({{{"employer", "xyz"}, {"region", "spb"}}}));
    }

    Y_UNIT_TEST(TwoContexts) {
        auto value = MakeSampleContextValue({
            {{{"region", "msk"}}, 20, 100},
            {{{"region", "spb"}}, 30, 200},
        });

        // Values are evaluated for both contexts independently, and only then aggregated.
        // Thus the result is defined by smallest value even if from weaker rule.
        UNIT_ASSERT_EQUAL(20, value.Resolve({
            {{"region", "msk"}},
            {{"region", "spb"}}
        }));
    }

}  // Y_UNIT_TEST_SUITE(TContextValue)
