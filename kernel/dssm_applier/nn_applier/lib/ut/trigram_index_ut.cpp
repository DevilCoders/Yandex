#include <kernel/dssm_applier/nn_applier/lib/trigram_index.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/memory/blob.h>
#include <util/random/mersenne.h>
#include <util/charset/wide.h>

using namespace NNeuralNetApplier;

namespace {
    TTermToIndex GetTermToIndex(TMersenne<ui64>& rng) {
        TTermToIndex trigrams;
        constexpr size_t numChars = 3;
        constexpr size_t numLetters = 150;

        for (size_t i = 0; i < 10000; ++i) {
            TUtf16String trigram;

            for (size_t j = 0; j < numChars; ++j) {
                trigram += static_cast<TUtf16String::char_type>(65 + rng.GenRand64() % numLetters);
            }

            trigrams[trigram] = i;
        }

        trigrams[u"abc"] = 213;
        return trigrams;
    }


    void TestIndexVsBaseLine(const TTrigramsIndex& testIndex, const TTermToIndex& baseline, TMersenne<ui64>& rng) {
        for (const auto& item : baseline) {
            size_t index = 0;
            UNIT_ASSERT(testIndex.Find(item.first, &index));
            UNIT_ASSERT(index == item.second);
        }

        for (const auto& item : GetTermToIndex(rng)) {
            size_t index = 0;
            UNIT_ASSERT(testIndex.Find(item.first, &index) == baseline.contains(item.first));
            if (baseline.contains(item.first)) {
                UNIT_ASSERT(index == baseline.at(item.first));
            }
        }
    }
}

Y_UNIT_TEST_SUITE(TrigramsIndexTest) {
    Y_UNIT_TEST(TestFindIndex) {
        TMersenne<ui64> rng(123243);
        const auto srcIndex = GetTermToIndex(rng);
        TTrigramsIndex testIndex(srcIndex);

        TestIndexVsBaseLine(testIndex, srcIndex, rng);
    }

    Y_UNIT_TEST(TestSaveLoad) {
        TMersenne<ui64> rng(123244);
        const auto srcIndex = GetTermToIndex(rng);
        TTrigramsIndex saveIndex(srcIndex);
        TStringStream holder;
        saveIndex.Save(&holder);

        TTrigramsIndex loadIndex;
        loadIndex.Init(TBlob::FromStream(holder));

        TestIndexVsBaseLine(loadIndex, srcIndex, rng);
    }
}
