#include <kernel/facts/word_set_match/word_set_match.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/split.h>

using namespace NFacts;

Y_UNIT_TEST_SUITE(WordSetMatch) {
    Y_UNIT_TEST(Smoke) {
        TWordSetStrictMatch matchSet;
        matchSet.AddWordSubset("бьет жену");
        matchSet.AddWordSubset("животное на букву");
        matchSet.AddWordSubset("бьет на");

        UNIT_ASSERT(matchSet.DoesSentenceMatch("сколько лет медведев бьет жену"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("сколько лет жену медведев бьет"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("букву ч на животное магистр йода загадал"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("животное на букву у бьет жену"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("сколько лет животное на букву у бьет животное на букву е и его жену"));

        UNIT_ASSERT(!matchSet.DoesSentenceMatch("__ фыва __++ , ? олдж"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("на"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("бьетна"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("набьет"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch(""));
    }

    Y_UNIT_TEST(WordsFromDifferentSets) {
        TWordSetStrictMatch matchSet;
        matchSet.AddWordSubset("бьет жену");
        matchSet.AddWordSubset("животное на букву");

        UNIT_ASSERT(!matchSet.DoesSentenceMatch("бьет бьет"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("животное бьет букву"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("животное бьет животное и букву"));
    }

    Y_UNIT_TEST(RepeatingWordsInSets) {
        TWordSetStrictMatch matchSet;
        matchSet.AddWordSubset("бьет жену бьет жену");
        matchSet.AddWordSubset("животное на букву на животное на букву");

        UNIT_ASSERT(matchSet.DoesSentenceMatch("бьет жену бьет жену"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("животное на букву у бьет на животное на букву о"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("букву букву на на на животное животное"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("букву букву на на на животное животное животное"));

        UNIT_ASSERT(!matchSet.DoesSentenceMatch("бьет жену"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("бьет жену бьет"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("жену бьет жену"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("бьет бьет бьет бьет"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("животное на на на на на на"));
    }

    Y_UNIT_TEST(TextNormalization) {
        TWordSetStrictMatch matchSet;
        matchSet.AddWordSubset("Бьет жЕну");

        UNIT_ASSERT(matchSet.DoesSentenceMatch("сколько лет медведев бьеТ жену"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("сколько лет Жену Медведев бьет"));

        UNIT_ASSERT(!matchSet.DoesSentenceMatch("сколько лет Жену Медведев бьёт"));
    }

    Y_UNIT_TEST(WithoutTextNormalization) {
        TWordSetStrictMatch matchSet;
        matchSet.AddWordSubset("Бьет жЕну", false);

        UNIT_ASSERT(matchSet.DoesSentenceMatch("сколько лет медведев Бьет жЕну", false));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("сколько лет медведев Бьет жЕну", true));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("сколько лет медведев бьеТ жену", true));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("сколько лет медведев бьеТ жену", false));
    }

    Y_UNIT_TEST(EmptySequences) {
        TWordSetStrictMatch matchSet;
        matchSet.AddWordSubset("Бьет  жЕну", false);

        UNIT_ASSERT(matchSet.DoesSentenceMatch("Бьет жЕну", false));
    }

    Y_UNIT_TEST(TooManyWordsInSubset) {
        TWordSetStrictMatch matchSet;
        const TString wordSet = "слово1 слово2 слово3 слово4 слово5 слово6 слово7 слово8 слово9";
        matchSet.AddWordSubset(wordSet);
        UNIT_ASSERT(StringSplitter(wordSet).Split(' ').Count() > TWordSetStrictMatch::MaxSubsetLen);

        const TString sentence1 = "слово1 слово2 слово3 слово4 слово5 слово6 слово7 слово8 слово9";
        UNIT_ASSERT(StringSplitter(sentence1).Split(' ').Count() > TWordSetStrictMatch::MaxSubsetLen);
        UNIT_ASSERT(matchSet.DoesSentenceMatch(sentence1));
        const TString sentence2 = "слово1 слово2 слово3 слово4 слово5 слово6 слово7 слово8";
        UNIT_ASSERT(StringSplitter(sentence2).Split(' ').Count() == TWordSetStrictMatch::MaxSubsetLen);
        UNIT_ASSERT(matchSet.DoesSentenceMatch(sentence2));

        const TString sentence3 = "слово1 слово2 слово3 слово4 слово5 слово6 слово7";
        UNIT_ASSERT(StringSplitter(sentence3).Split(' ').Count() < TWordSetStrictMatch::MaxSubsetLen);
        UNIT_ASSERT(!matchSet.DoesSentenceMatch(sentence3));
    }

    Y_UNIT_TEST(SubstringMatch) {
        TWordSetSubstringMatch matchSet;
        matchSet.AddWordSubset("бьет жен");
        matchSet.AddWordSubset("животн букв");
        matchSet.Finalize();

        UNIT_ASSERT(matchSet.DoesSentenceMatch("бьетжен"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("женбьет"));
        UNIT_ASSERT(matchSet.DoesSentenceMatch("недоживотное на букву б"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("живот ное на букву у"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("животное на бук ву э"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("бьет бьет"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch("жену жену жену"));
    }

    Y_UNIT_TEST(NotFinalizedSubstringMatch) {
        TWordSetSubstringMatch matchSet;
        matchSet.AddWordSubset("животн букв");

        UNIT_ASSERT(!matchSet.IsInitialized());
        UNIT_ASSERT_EXCEPTION_CONTAINS(matchSet.DoesSentenceMatch("животное на букву"), yexception, "Please finalize the strategy before searching");
    }

    Y_UNIT_TEST(EmptySubstringMatch) {
        TWordSetSubstringMatch matchSet;
        matchSet.Finalize();

        UNIT_ASSERT(!matchSet.DoesSentenceMatch("животное на букву"));
        UNIT_ASSERT(!matchSet.DoesSentenceMatch(""));
    }

    Y_UNIT_TEST(FindWordSetNumber) {
        TWordSetSubstringMatch matchSet;

        matchSet.AddWordSubset("бьет жен");
        matchSet.AddWordSubset("животн букв у");
        matchSet.AddWordSubset("животн букву у");
        matchSet.AddWordSubset("гы гы гы");
        matchSet.Finalize();

        const auto* const matchingWordSet = matchSet.GetFirstMatchingWordSet("бьетжен");
        UNIT_ASSERT(matchingWordSet != nullptr);
        UNIT_ASSERT(matchingWordSet->size() == 2);

        const auto* const matchingWordSet2 = matchSet.GetFirstMatchingWordSet("животное на букву у");
        UNIT_ASSERT(matchingWordSet2 != nullptr);
        UNIT_ASSERT(matchingWordSet2->size() == 3);
        UNIT_ASSERT_STRINGS_UNEQUAL((*matchingWordSet2)[0], "гы");
        UNIT_ASSERT_STRINGS_UNEQUAL((*matchingWordSet2)[1], "гы");
        UNIT_ASSERT_STRINGS_UNEQUAL((*matchingWordSet2)[2], "гы");
    }
}

