#include "offroad_hashed_keyinv_wad_ut.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TestOffroadHashedKeyInvWad) {
    static constexpr ui32 Ui64NumBits = 56;
    static constexpr ui32 Ui32NumBits = 32;

    template<class Io>
    void Test(ui32 numHashes, ui32 numDocs, ui32 numBits, bool useFakeRng = false) {
        using TSearcher = typename TIndexGenerator<Io>::TSearcher;
        using TData = typename TIndexGenerator<Io>::TData;
        using TIterator = typename TIndexGenerator<Io>::TIterator;
        using TInt = typename TIndexGenerator<Io>::TInt;

        TIndexGenerator<Io> generator(numHashes, numDocs, numBits, useFakeRng);
        TSearcher searcher = generator.GetSearcher();
        TData data = generator.GetData();

        UNIT_ASSERT_EQUAL(searcher.Size(), data.size());

        TFastRng32 rng58(58, 0);
        Shuffle(data.begin(), data.end(), rng58);
        TIterator iterator;
        TVector<TIterator> iterators;
        TVector<bool> found;

        THashSet<TInt> hashes;
        TMap<TInt, TVector<TMaybe<size_t>>> hashesPositions;

        size_t keyIndex = 0;
        for (auto&& [hash, documents] : data) {
            hashesPositions[hash].push_back(keyIndex);
            hashes.insert(hash);
            UNIT_ASSERT(searcher.Find(hash, &iterator));
            TVector<TPantherHit> docs;
            while (iterator.ReadHits([&](TPantherHit hit) {
                docs.push_back(hit);
                return true;
            })) {}
            UNIT_ASSERT_EQUAL(docs, documents);
            ++keyIndex;
        }

        TFastRng<TInt> otherHashes((4243 + numHashes) ^ numDocs);
        TVector<TInt> manyOtherHashes(Reserve(numHashes));
        for (size_t i = 0; i < numHashes; ++i) {
            ui64 hash = otherHashes.GenRand();
            while (hashes.contains(hash)) {
                hash = otherHashes.GenRand();
            }
            manyOtherHashes.push_back(hash);
            hashesPositions[hash].push_back(Nothing());
            UNIT_ASSERT(!searcher.Find(hash, &iterator));
        }

        Sort(manyOtherHashes.begin(), manyOtherHashes.end());
        iterators.clear();
        TVector<bool> otherFound = searcher.FindMany(manyOtherHashes, &iterators);
        UNIT_ASSERT(AllOf(otherFound, [](bool val) { return val == false; }));

        TVector<TInt> manyHashes(Reserve(keyIndex + numHashes));
        TVector<TMaybe<size_t>> positions(Reserve(keyIndex + numHashes));

        for (auto&& [hash, equalHashPositions] : hashesPositions) {
            for (auto&& position : equalHashPositions) {
                manyHashes.push_back(hash);
                positions.push_back(position);
            }
        }

        found = searcher.FindMany(manyHashes, &iterators);
        for (size_t i = 0; i < manyHashes.size(); ++i) {
            bool hasToBeFound = positions[i].Defined();
            UNIT_ASSERT(found[i] == hasToBeFound);
            if (hasToBeFound) {
                TVector<TPantherHit> docs;
                while (iterators[i].ReadHits([&](TPantherHit hit) {
                    docs.push_back(hit);
                    return true;
                })) {}
                UNIT_ASSERT_EQUAL(docs, data[*positions[i]].second);
            }
        }
    }

    Y_UNIT_TEST(Small) {
        Test<TUi64Io>(5, 5, Ui64NumBits);
        Test<TUi32Io>(5, 5, Ui32NumBits);
    }

    Y_UNIT_TEST(NotSoBig) {
        for (ui32 i = 120; i <= 130; ++i) {
            for (ui32 j = 120; j <= 130; ++j) {
                Test<TUi64Io>(i, j, Ui64NumBits);
                Test<TUi32Io>(i, j, Ui32NumBits);
            }
        }
    }

    Y_UNIT_TEST(Bigger) {
        Test<TUi64Io>(5, 50000, Ui64NumBits);
        Test<TUi32Io>(5, 50000, Ui32NumBits);
    }

    Y_UNIT_TEST(MuchBigger) {
        Test<TUi64Io>(50000, 5, Ui64NumBits);
        Test<TUi32Io>(50000, 5, Ui32NumBits);
    }

    Y_UNIT_TEST(MuchBiggerWithFakeRng) {
        Test<TUi64Io>(50000, 5, Ui64NumBits, true);
        Test<TUi32Io>(50000, 5, Ui32NumBits, true);
    }

    Y_UNIT_TEST(MuchMoreBigger) {
        Test<TUi64Io>(5000, 5000, Ui64NumBits);
        Test<TUi32Io>(5000, 5000, Ui32NumBits);
    }
}
