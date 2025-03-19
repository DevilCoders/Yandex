#include <kernel/stringmatch_tracker/matchers/matcher.h>
#include <library/cpp/testing/unittest/registar.h>

/*
 * Tests for TLCSMatcher
 */

namespace {
    const size_t DEFAULT_THRESHOLD = 3;

    class TReverseStringReader {
    private:
        const TString& Str;
        size_t CurIndex = 0;
    public:
        TReverseStringReader(const TString& str)
            : Str(str)
            , CurIndex(str.size())
        {
        }

        char GetPreviousChar() {
            if (!CurIndex) {
                return 0;
            }
            return Str[--CurIndex];
        }
    };

    size_t CalcMatch(const TString& str, NSequences::TLCSMatcher& matcher) {
        TReverseStringReader reader(str);
        while (char c = reader.GetPreviousChar()) {
            matcher.ExtendMatch(c);
        }
        size_t match = matcher.GetCurMatch();
        matcher.ResetMatch();
        return match;
    }

};

Y_UNIT_TEST_SUITE(TLCSMatcherTest) {

    // Tests matching with empty basic or additional string
    Y_UNIT_TEST(EmptyRequestOrUrl) {
        NSequences::TLCSMatcher emptyMatcherDefault("", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("", emptyMatcherDefault), 0);
        UNIT_ASSERT_EQUAL(CalcMatch("hello", emptyMatcherDefault), 0);
        UNIT_ASSERT_EQUAL(CalcMatch("one more test", emptyMatcherDefault), 0);

        NSequences::TLCSMatcher zeroMatcher("simple request");
        UNIT_ASSERT_EQUAL(CalcMatch("", zeroMatcher), 0);
    }

    // Tests correctness on usual examples
    Y_UNIT_TEST(CorrectnessTest) {
        NSequences::TLCSMatcher matcherDefault("hello priv'et", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("llop", matcherDefault), 4);
        UNIT_ASSERT_EQUAL(CalcMatch("belroprivek", matcherDefault), 6);
        UNIT_ASSERT_EQUAL(CalcMatch("tellzx", matcherDefault), 3); // eq threshold
        UNIT_ASSERT_EQUAL(CalcMatch("telzx", matcherDefault), 0); // less than threshold

        NSequences::TLCSMatcher matcherZero("hello priv'et");
        UNIT_ASSERT_EQUAL(CalcMatch("telzx", matcherZero), 2);
    }

    // Tests correctness if match is empty (and strings are not)
    Y_UNIT_TEST(ZeroMatchTest) {
        NSequences::TLCSMatcher matcherDefault("abcdefghij", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("qwrty", matcherDefault), 0);

        NSequences::TLCSMatcher matcherZero("abcdefghij");
        UNIT_ASSERT_EQUAL(CalcMatch("qwrty", matcherDefault), 0);
    }

    // Small matches
    Y_UNIT_TEST(SmallMatchTest) {
        NSequences::TLCSMatcher matcherDefault("hello friends", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("str another str", matcherDefault), 0);
        UNIT_ASSERT_EQUAL(CalcMatch("htons", matcherDefault), 0);
        UNIT_ASSERT_EQUAL(CalcMatch("test-en*ing", matcherDefault), 0);
        UNIT_ASSERT_EQUAL(CalcMatch("test-ending", matcherDefault), 3);

        NSequences::TLCSMatcher zeroMatcher("hello friends");
        UNIT_ASSERT_EQUAL(CalcMatch("str another str", zeroMatcher), 2);
        UNIT_ASSERT_EQUAL(CalcMatch("htons", zeroMatcher), 1);
        UNIT_ASSERT_EQUAL(CalcMatch("test-en*ing", zeroMatcher), 2);
        UNIT_ASSERT_EQUAL(CalcMatch("test-ending", zeroMatcher), 3);
    }

    // Small Input Strings
    Y_UNIT_TEST(SmallInputTest) {
        NSequences::TLCSMatcher matcherDefault1("hi", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("anystring", matcherDefault1), 0);

        NSequences::TLCSMatcher matcherDefault2("h&i!@#", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("h&i!@#!!word*!anotherword", matcherDefault2), 0);

        NSequences::TLCSMatcher matcherDefault3("hi !@ fiend", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("s*   m^!@#11", matcherDefault3), 0); // sm - "small"
        UNIT_ASSERT_EQUAL(CalcMatch("sm", matcherDefault3), 0); // sm - "small"
    }

    // Not-english symbols in input (if transliteration fails for any reason)
    Y_UNIT_TEST(BadInputTest) {
        NSequences::TLCSMatcher matcherDefault1("нетранслитерированная строка", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("другая нетранслитерированная строка", matcherDefault1), 0);

        NSequences::TLCSMatcher matcherDefault2("частично transliterated строка", DEFAULT_THRESHOLD);
        UNIT_ASSERT_EQUAL(CalcMatch("другая частично тransлитерированная string ", matcherDefault2), 4);
        // 3rd letter in трanслитерированная is english letter (a)
        UNIT_ASSERT_EQUAL(CalcMatch("еще одна частично трanслитерированная string ", matcherDefault2), 3);
        // 3rd letter in траnsлитерированнаya is russian letter (a)
        UNIT_ASSERT_EQUAL(CalcMatch("и еще одна частично траnsлитерированнаya string ", matcherDefault2), 0);
    }
}
