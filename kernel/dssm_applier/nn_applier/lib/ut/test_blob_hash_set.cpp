#include <kernel/dssm_applier/nn_applier/lib/blob_hash_set.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/stream/str.h>


using namespace NNeuralNetApplier;

namespace {

template<class T>
void CheckCorrectness(const TBlobHashSet<T>& blobHashSet, const THashSet<T>& correctHashSet, const TVector<T>& keys) {
    for (const T& key : keys) {
        bool foundCorrect = correctHashSet.contains(key);
        bool found = blobHashSet.Contains(key);
        if (foundCorrect) {
            UNIT_ASSERT_C(found, "THashSet contains '" << key << "', but TBlobHashSet is not");
        } else {
            UNIT_ASSERT_C(!found, "THashSet does not contain '" << key << "', but TBlobHashSet does");
        }
    }
}

TVector<ui64> GenerateRandomSequence(size_t size, ui32 seed = 42) {
    TVector<ui64> result;
    TReallyFastRng32 rng(seed);
    for (size_t i = 0; i < size; ++i) {
        result.push_back(rng.GenRand64());
    }
    return result;
}

}  // unnamed namespace

Y_UNIT_TEST_SUITE(TBlobHashSetCorrectness) {

Y_UNIT_TEST(CorrectnessSmall) {
    THashSet<ui32> correctHashSet;
    TVector<ui32> keys;
    for (size_t i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            correctHashSet.insert(i);
        }
        keys.push_back(i);
    }

    TBlobHashSet<ui32> blobHashSet = TBlobHashSet<ui32>::FromContainer(correctHashSet);
    CheckCorrectness(blobHashSet, correctHashSet, keys);
}

Y_UNIT_TEST(CorrectnessBig) {
    size_t sampleSize = 1'000'000;
    size_t setSize = sampleSize / 2;

    TVector<ui64> keys = GenerateRandomSequence(sampleSize, 42);
    THashSet<ui64> correctHashSet(keys.begin(), keys.begin() + setSize);

    TBlobHashSet<ui64> blobHashSet = TBlobHashSet<ui64>::FromContainer(correctHashSet);

    TReallyFastRng32 rng(13);
    Shuffle(keys.begin(), keys.end(), rng);

    CheckCorrectness(blobHashSet, correctHashSet, keys);
}

Y_UNIT_TEST(TestEmptyMarker) {
    ui32 emptyMarker = Max<ui32>();
    UNIT_ASSERT_VALUES_EQUAL(emptyMarker, NDefaultTraits::TDefaultTraits<ui32>::EmptyMarker);
    TVector<ui32> keys = {0, 1, emptyMarker - 1, emptyMarker};
    THashSet<ui32> correctHashSet(keys.begin(), keys.end());
    TBlobHashSet<ui32> set = TBlobHashSet<ui32>::FromContainer(correctHashSet);
    UNIT_ASSERT_C(set.Contains(0), "set must contain 0");
    UNIT_ASSERT_C(set.Contains(1), "set must contain 0");
    UNIT_ASSERT_C(set.Contains(emptyMarker - 1), "set must contain emptyMarker - 1 = " << emptyMarker - 1);
    UNIT_ASSERT_C(!set.Contains(emptyMarker), "set must not contain emptyMarker = " << emptyMarker);
}

Y_UNIT_TEST(TestFailOnBadScaleFactor) {
    size_t dataSize = 1'000'000;
    TVector<ui32> keys(dataSize);
    try {
        TBlobHashSet<ui32> set = TBlobHashSet<ui32>::FromContainer(keys, 0.99);
        UNIT_ASSERT_C(false, "the error should have been thrown on previous line");
    } catch (...) {

    }
}

}  // TestCorrectness

Y_UNIT_TEST_SUITE(TBlobHashSetSaveLoad) {

Y_UNIT_TEST(SaveLoadSmall) {
    THashSet<ui32> correctHashSet;
    TVector<ui32> keys;
    for (size_t i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            correctHashSet.insert(i);
        }
        keys.push_back(i);
    }

    TStringStream ss;
    {
        TBlobHashSet<ui32> blobHashSet = TBlobHashSet<ui32>::FromContainer(correctHashSet);
        blobHashSet.Save(&ss);
    }

    TBlobHashSet<ui32> loadedBlobHashSet;
    loadedBlobHashSet.Load(TBlob::FromStream(ss));

    CheckCorrectness(loadedBlobHashSet, correctHashSet, keys);
}

void SaveLoadTestSuit(const size_t sampleSize) {
    size_t setSize = sampleSize / 2;

    TVector<ui64> keys = GenerateRandomSequence(sampleSize, 42 + sampleSize);
    THashSet<ui64> correctHashSet(keys.begin(), keys.begin() + setSize);

    TStringStream ss;
    {
        TBlobHashSet<ui64> blobHashSet = TBlobHashSet<ui64>::FromContainer(correctHashSet);
        blobHashSet.Save(&ss);
    }

    TBlobHashSet<ui64> loadedBlobHashSet;
    loadedBlobHashSet.Load(TBlob::FromStream(ss));

    CheckCorrectness(loadedBlobHashSet, correctHashSet, keys);
}

Y_UNIT_TEST(SaveLoadBig) {
    for (size_t sampleSize : {100, 1000, 10'000, 100'000, 1000'000}) {
        SaveLoadTestSuit(sampleSize);
    }
}

}  // TestSaveLoad
