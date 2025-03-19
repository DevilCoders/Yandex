#include "offroad_block_string_wad_ut.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestBlockStringWad) {
    template<class Io>
    void Test(ui32 numHashes) {
        using TSearcher = typename TIndexGenerator<Io>::TSearcher;
        using TData = typename TIndexGenerator<Io>::TData;
        using TDataRefs = typename TIndexGenerator<Io>::TDataRefs;
        using TReader = typename TIndexGenerator<Io>::TReader;

        TIndexGenerator<Io> generator(numHashes);
        TSearcher searcher = generator.GetSearcher();
        TVector<TStringBuf> sortedDataRefs = generator.GetDataRefs();
        TVector<TStringBuf> dataRefs = sortedDataRefs;

        TFastRng<ui32> rand(numHashes + 43242);
        TFastRng32 rng58(58, 0);
        Shuffle(dataRefs.begin(), dataRefs.end(), rng58);

        THashSet<TString> hashes;

        for (const auto& hash : dataRefs) {
            hashes.insert(TString(hash.Data(), hash.size()));
            ui32 hashId = 0;
            UNIT_ASSERT(searcher.Find(hash, &hashId));
            UNIT_ASSERT_EQUAL(sortedDataRefs[hashId], hash);
        }

        TData otherHashes(Reserve(numHashes));
        TDataRefs otherHashesRefs(Reserve(numHashes));
        while (otherHashesRefs.size() < numHashes) {
            TString current = TIndexGenerator<Io>::GenerateString(rand, numHashes);
            TStringBuf currentRef = current;
            if (!hashes.find(currentRef)) {
                otherHashes.push_back(current);
                otherHashesRefs.push_back(currentRef);
                ui32 hashId = 0;
                UNIT_ASSERT(!searcher.Find(currentRef, &hashId));
            }
        }

        Sort(otherHashesRefs.begin(), otherHashesRefs.end());
        Sort(otherHashes.begin(), otherHashes.end());
        TVector<ui32> hashIds(otherHashesRefs.size());
        TVector<bool> otherFound = searcher.Find(otherHashesRefs, hashIds);
        UNIT_ASSERT(AllOf(otherFound, [](bool val) { return val == false; }));

        TDataRefs manyHashes(Reserve(otherHashesRefs.size() + sortedDataRefs.size()));
        std::merge(otherHashesRefs.begin(), otherHashesRefs.end(), sortedDataRefs.begin(), sortedDataRefs.end(), std::back_inserter(manyHashes));

        TVector<ui32> manyHashIds(manyHashes.size());
        TVector<bool> manyFound = searcher.Find(manyHashes, manyHashIds);

        for (size_t i = 0; i < manyHashes.size(); ++i) {
            auto it = hashes.find(manyHashes[i]);
            bool hasToBeFound = it != (hashes.end());
            UNIT_ASSERT_EQUAL(hasToBeFound, manyFound[i]);
            if (hasToBeFound) {
                UNIT_ASSERT_EQUAL(manyHashes[i], sortedDataRefs[manyHashIds[i]]);
            }
        }

        TStringBuf hash;
        TReader reader = generator.GetReader();
        size_t i = 0;
        for (i = 0; i < sortedDataRefs.size(); ++i) {
            UNIT_ASSERT(reader.ReadHash(&hash));
            UNIT_ASSERT_EQUAL(hash, sortedDataRefs[i]);
        }
        UNIT_ASSERT(!reader.ReadHash(&hash));

        UNIT_ASSERT_EQUAL(searcher.Size(), i);
    }

    Y_UNIT_TEST(Small) {
        Test<TBlockStringIo>(20);
    }

    Y_UNIT_TEST(NotSoBig) {
        for (ui32 i = 1200; i <= 1300; ++i) {
            Test<TBlockStringIo>(i);
        }
    }

    Y_UNIT_TEST(Bigger) {
        Test<TBlockStringIo>(10000);
    }

    Y_UNIT_TEST(MuchBigger) {
        Test<TBlockStringIo>(20000);
    }

    Y_UNIT_TEST(MuchMoreBigger) {
        Test<TBlockStringIo>(50000);
    }
}
