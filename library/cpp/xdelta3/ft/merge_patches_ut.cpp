#include <library/cpp/testing/gtest/gtest.h>

#include <library/cpp/xdelta3/state/data_ptr.h>
#include <library/cpp/xdelta3/ut/rand_data/generator.h>
#include <library/cpp/xdelta3/xdelta_codec/codec.h>

#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>

#include <algorithm>
#include <string.h>

using namespace NXdeltaAggregateColumn;

constexpr auto MaxDataSize = 3000000;
constexpr size_t N = 500;

struct TPatch {
    TDataPtr Data;
    size_t Size;
};

TEST(XDeltaMergePatches, RandomMerge) 
{
    size_t patchCount = N;

    TMap<size_t, TPatch> patches;

    auto init = TDataPtr();
    size_t initSize = 0;
    auto a = TDataPtr(); // copy of init because zero size
    size_t sizeA = initSize;

    size_t sizeB = 0;
    TDataPtr b;

    for (auto i : xrange(patchCount)) {
        sizeB = rand() % MaxDataSize;
        b = RandDataModification(
            a.get(), 
            sizeA, 
            sizeB);

        size_t patchSize = 0;
        auto patch = TDataPtr(ComputePatch(nullptr, a.get(), sizeA, b.get(), sizeB, &patchSize));

        patches[i] = {std::move(patch), patchSize};

        a = std::move(b);
        sizeA = sizeB;

        ++i;
    }

    for (;patchCount > 1; --patchCount) {
        size_t i = rand() % N;

        auto it = patches.lower_bound(i);
        if (it == patches.end()) {
            --it;
            --it;
        } else if (std::next(it) == patches.end()) {
            --it;
        }

        auto left = it;
        auto patch1 = left->second.Data.get();
        auto patch1Size = left->second.Size;
        auto right = ++it;
        auto patch2 = right->second.Data.get();
        auto patch2Size = right->second.Size;

        size_t patchSize = 0;
        auto patch = TDataPtr(MergePatches(nullptr, 0, patch1, patch1Size, patch2, patch2Size, &patchSize));
        EXPECT_TRUE(patchSize > 0 && patch);

        patches.erase(right);
        left->second.Data = std::move(patch);
        left->second.Size = patchSize;
    }

    size_t resultSize = 0;
    auto result = TDataPtr(ApplyPatch(nullptr, 0, init.get(), initSize, patches.begin()->second.Data.get(), patches.begin()->second.Size, sizeB, &resultSize));
    EXPECT_TRUE(resultSize == sizeB);
    EXPECT_EQ(0, memcmp(result.get(), a.get(), resultSize));
}

TEST(XDeltaMergePatches, SerialMerge) {
    size_t baseSize = rand() % MaxDataSize + 100;
    auto base = TDataPtr(RandData(baseSize));
    size_t initSize = baseSize;
    TVector<ui8> init(base.get(), base.get() + baseSize);

    size_t patchSize = 0;
    TDataPtr patch;

    for (size_t i = 0; i < N; ++i) {
        size_t derivedSize = baseSize + (rand() % 2 == 1 ? -1 : 1) * std::min(static_cast<int>(baseSize), static_cast<int>(rand()) % 100);

        auto derived = TDataPtr(RandDataModification(base.get(), baseSize, derivedSize));

        size_t newPatchSize = 0;
        auto newPatch = TDataPtr(ComputePatch(nullptr, base.get(), baseSize, derived.get(), derivedSize, &newPatchSize));
        EXPECT_TRUE(newPatch && newPatchSize > 0);

        if (patch) {
            size_t mergedPatchSize = 0;
            auto mergedPatch = TDataPtr(MergePatches(nullptr, 0, patch.get(), patchSize, newPatch.get(), newPatchSize, &mergedPatchSize));

            EXPECT_TRUE(mergedPatch && mergedPatchSize > 0);

            size_t resultSize = 0;

            auto result = TDataPtr(ApplyPatch(nullptr, 0, &init[0], initSize, mergedPatch.get(), mergedPatchSize, derivedSize, &resultSize));

            EXPECT_TRUE(result);
            EXPECT_TRUE(resultSize == derivedSize);
            EXPECT_EQ(0, memcmp(derived.get(), result.get(), derivedSize));

            patch = std::move(mergedPatch);
            patchSize = mergedPatchSize;

        } else {
            patch = std::move(newPatch);
            patchSize = newPatchSize;
        }

        base = std::move(derived);
        baseSize = derivedSize;
    }
}

TEST(XDeltaMergePatches, EncodeDecode) {
    for (size_t i = 0; i < N; ++i) {
        size_t baseSize = rand() % MaxDataSize;
        auto base = RandData(baseSize);

        size_t derived1Size = rand() % MaxDataSize;
        auto derived1 = RandData(derived1Size);

        size_t patch1Size;
        auto patch1 = TDataPtr(ComputePatch(nullptr, base.get(), baseSize, derived1.get(), derived1Size, &patch1Size));

        size_t resultSize = 0;
        auto result = TDataPtr(ApplyPatch(nullptr, 0, base.get(), baseSize, patch1.get(), patch1Size, derived1Size, &resultSize));

        EXPECT_TRUE(result.get() != nullptr && resultSize == derived1Size);
        EXPECT_EQ(0, memcmp(result.get(), derived1.get(), resultSize));
    }
}
