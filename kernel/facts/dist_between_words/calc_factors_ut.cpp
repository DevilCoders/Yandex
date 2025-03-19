#include "calc_factors.h"

#include <library/cpp/charset/wide.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NDistBetweenWords;

namespace {
    TWordDistTriePtr BuildTrie(TBufferOutput& buftmp, std::function<void (TWordDistTrieBuilder&)> updater) {
        TWordDistTrieBuilder builder;
        updater(builder);
        builder.Save(buftmp);
        return MakeAtomicShared<TWordDistTrie>(buftmp.Buffer().Data(), buftmp.Buffer().Size());
    }
}

Y_UNIT_TEST_SUITE(WordDistFactor) {
    Y_UNIT_TEST(BaseCalculation) {
        TBufferOutput buftmp;
        TWordDistTriePtr trie = BuildTrie(buftmp, [](TWordDistTrieBuilder& builder) {
            builder.Add(BuildKey("как"), TTrieData{0.f, 1.f, 2.f, 3.f});
        });

        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).Calc("", "как"), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost10(trie).Calc("", "как"), 1);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildUrl5(trie).Calc("", "как"), 2);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildUrl10(trie).Calc("", "как"), 3);

        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).Calc("как", ""), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost10(trie).Calc("как", ""), 1);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildUrl5(trie).Calc("как", ""), 2);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildUrl10(trie).Calc("как", ""), 3);

        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).Calc("", ""), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost10(trie).Calc("", ""), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildUrl5(trie).Calc("", ""), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildUrl10(trie).Calc("", ""), 0);
    }

    Y_UNIT_TEST(ComplexCalculation) {
        TBufferOutput buftmp;
        TWordDistTriePtr trie = BuildTrie(buftmp, [](TWordDistTrieBuilder& builder) {
            builder.Add(BuildKey("замри"), TTrieData{0.8f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("застынь"), TTrieData{0.769f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("мгновение"), TTrieData{0.65f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("замри", "застынь"), TTrieData{0.2f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("замри", "мгновение"), TTrieData{0.8f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("застынь", "мгновение"), TTrieData{0.95f, 0.f, 0.f, 0.f});
        });

        UNIT_ASSERT_DOUBLES_EQUAL(TCalcFactor::BuildHost5(trie).Calc("замри мгновение", "застынь мгновение"), 0.2, 0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(TCalcFactor::BuildHost5(trie).Calc("замри", "застынь мгновение"), 0.85, 0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(TCalcFactor::BuildHost5(trie).Calc("", "застынь мгновение"), 1.419, 0.0001);
    }

    Y_UNIT_TEST(BigramBaseCalculation) {
        TBufferOutput buftmp;
        TWordDistTriePtr trie = BuildTrie(buftmp, [](TWordDistTrieBuilder& builder) {
            builder.Add(BuildKey("как"), TTrieData{0.f, 1.f, 2.f, 3.f});
            builder.Add(BuildKey("как так"), TTrieData{0.f, 1.f, 2.f, 3.f});
        });

        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("", "как"), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("", "как так"), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("как так", ""), 0);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("как ток", ""), 1);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("ток ток", ""), 2);
        UNIT_ASSERT_VALUES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("", "ток ток"), 2);
    }

    Y_UNIT_TEST(BigramComplexCalculation) {
        TBufferOutput buftmp;
        TWordDistTriePtr trie = BuildTrie(buftmp, [](TWordDistTrieBuilder& builder) {
            builder.Add(BuildKey("мальдивы"), TTrieData{0.02f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("что такое", "это"), TTrieData{0.5f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("уголовном кодексе", "ук"), TTrieData{0.6673150743789701f, 0.f, 0.f, 0.f});
            builder.Add(BuildKey("кодексе россии", "россии матушки"), TTrieData{0.8f, 0.f, 0.f, 0.f});
        });

        UNIT_ASSERT_DOUBLES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("мальдивы это", "что такое мальдивы"), 0.5 + 2 * 0.02, 0.0001);
        UNIT_ASSERT_DOUBLES_EQUAL(TCalcFactor::BuildHost5(trie).CalcBigram("ук россии матушки", "уголовном кодексе россии"), 1 + 0.6673150743789701f, 0.0001);
    }
}
