#include <library/cpp/testing/unittest/registar.h>

#include "keyinv_portions_key_ranges.h"

#include <random>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestKeyInvPortionsKeyRanges) {
    Y_UNIT_TEST(SingleRange) {
        TKeyInvPortionsKeyRanges ranges({ "" });
        UNIT_ASSERT_VALUES_EQUAL(0, ranges.Range("1"));
        UNIT_ASSERT_VALUES_EQUAL(0, ranges.Range("a"));
        UNIT_ASSERT_VALUES_EQUAL(0, ranges.Range("z"));
    }

    Y_UNIT_TEST(Simple) {
        TKeyInvPortionsKeyRanges ranges({ "", "a", "ac", "b", "h", "x" });
        TString tmp;
        tmp.push_back('a' - 1);
        UNIT_ASSERT_VALUES_EQUAL(0, ranges.Range(tmp));
        UNIT_ASSERT_VALUES_EQUAL(1, ranges.Range("a"));
        UNIT_ASSERT_VALUES_EQUAL(1, ranges.Range("ab"));
        UNIT_ASSERT_VALUES_EQUAL(2, ranges.Range("ac"));
        UNIT_ASSERT_VALUES_EQUAL(2, ranges.Range("ad"));
        UNIT_ASSERT_VALUES_EQUAL(3, ranges.Range("c"));
        UNIT_ASSERT_VALUES_EQUAL(3, ranges.Range("d"));
        UNIT_ASSERT_VALUES_EQUAL(4, ranges.Range("i"));
        UNIT_ASSERT_VALUES_EQUAL(5, ranges.Range("z"));

        // test save/load
        TString buf;
        TStringOutput out(buf);
        Save(&out, ranges);
        TKeyInvPortionsKeyRanges ranges2;
        TStringInput in(buf);
        Load(&in, ranges2);
        UNIT_ASSERT_VALUES_EQUAL(ranges.StartKeys(), ranges2.StartKeys());
        UNIT_ASSERT_VALUES_EQUAL(0, ranges.Range(tmp));
        UNIT_ASSERT_VALUES_EQUAL(1, ranges.Range("a"));
        UNIT_ASSERT_VALUES_EQUAL(1, ranges.Range("ab"));
        UNIT_ASSERT_VALUES_EQUAL(2, ranges.Range("ac"));
        UNIT_ASSERT_VALUES_EQUAL(2, ranges.Range("ad"));
        UNIT_ASSERT_VALUES_EQUAL(3, ranges.Range("c"));
        UNIT_ASSERT_VALUES_EQUAL(3, ranges.Range("d"));
        UNIT_ASSERT_VALUES_EQUAL(4, ranges.Range("i"));
        UNIT_ASSERT_VALUES_EQUAL(5, ranges.Range("z"));
    }

    static const TVector<ui32> Chars = { 0, 1, 2, 3, 97, 98, 256 };

    TString GenerateRandomString() {
        size_t len = 1 + rand() % 5;
        TString s;
        for (size_t i = 0; i < len; ++i) {
            s.push_back(Chars[rand() % Chars.size()]);
        }
        return s;
    }

    Y_UNIT_TEST(Random) {
        std::minstd_rand rand(4243);
        for (size_t ranges = 1; ranges <= 100; ++ranges) {
            for (size_t it = 0; it < 32; ++it) {
                THashSet<TString> rangesSet;
                rangesSet.insert("");
                while (rangesSet.size() < ranges) {
                    rangesSet.insert(GenerateRandomString());
                }
                TVector<TString> rangesList(rangesSet.begin(), rangesSet.end());
                Sort(rangesList);
                TKeyInvPortionsKeyRanges keyInvRanges(rangesList);
                rangesSet.erase("");
                while (rangesSet.size() < ranges * 3) {
                    rangesSet.insert(GenerateRandomString());
                }
                for (const auto& s : rangesSet) {
                    size_t expected = rangesList.size() - 1;
                    for (size_t i = 0; i + 1 < rangesList.size(); ++i) {
                        if (rangesList[i] <= s && s < rangesList[i + 1]) {
                            expected = i;
                            break;
                        }
                    }
                    UNIT_ASSERT_VALUES_EQUAL(expected, keyInvRanges.Range(s));
                }
            }
        }
    }
}
