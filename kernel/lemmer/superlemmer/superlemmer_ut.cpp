#include "superlemmer.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

#include <initializer_list>

class TTestSuperLemmer: public TTestBase {
    UNIT_TEST_SUITE(TTestSuperLemmer);
    UNIT_TEST(TestApply);
    UNIT_TEST(TestReplace);
    UNIT_TEST_SUITE_END();

    THashMap<TString, TString> TestData;

public:
    TTestSuperLemmer()
        : TestData({{"мороженое", "мороженый"},
                    {"мороженый", "мороженый"},
                    {"уж", "узкий"},
                    {"уже", "узкий"},
                    {"узкий", "узкий"},
                    {"хорошо", "хороший"},
                    {"хороший", "хороший"},
                    {"yandex", "yandex"}}) {
    }

    void TestApply() {
        for (const auto& pair : TestData) {
            UNIT_ASSERT_EQUAL(NSuperLemmer::Apply(pair.first), pair.second);
        }
    }

    void TestReplace() {
        for (const auto& pair : TestData) {
            auto key = pair.first;
            NSuperLemmer::ApplyInplace(key);
            UNIT_ASSERT_EQUAL(key, pair.second);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestSuperLemmer);
