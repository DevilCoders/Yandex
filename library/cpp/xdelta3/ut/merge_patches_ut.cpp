#include <library/cpp/testing/gtest/gtest.h>

#include <library/cpp/xdelta3/xdelta_codec/codec.h>

#include <library/cpp/xdelta3/ut/rand_data/generator.h>

#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>

#include <string.h>

using namespace NXdeltaAggregateColumn;

constexpr auto MaxDataSize = 3000000;

TEST(XDeltaMergePatches, EncodeDecodeSimple)
{
    size_t baseSize = rand() % MaxDataSize;
    auto base = RandData(baseSize);

    size_t derived1Size = rand() % MaxDataSize;
    auto derived1 = RandDataModification(base.get(), baseSize, derived1Size);

    size_t patch1Size;
    auto patch1 = TDataPtr(ComputePatch(nullptr, base.get(), baseSize, derived1.get(), derived1Size, &patch1Size));

    size_t resultSize = 0;
    auto result = TDataPtr(ApplyPatch(nullptr, 0, base.get(), baseSize, patch1.get(), patch1Size, derived1Size, &resultSize));

    EXPECT_TRUE(result.get() != nullptr && resultSize == derived1Size);
    EXPECT_EQ(0, memcmp(result.get(), derived1.get(), resultSize));
}

TEST(XDeltaMergePatches, EmptyBase)
{
    const char* base = "";
    const char* derived1 = "0987654321";
    size_t derived1Size = strlen(derived1);
    size_t patch12Size = 0;
    auto patch12 = TDataPtr(ComputePatch(nullptr, reinterpret_cast<const ui8*>(base), strlen(base), reinterpret_cast<const ui8*>(derived1), derived1Size, &patch12Size));

    EXPECT_TRUE(patch12.get() != nullptr && patch12Size);

    size_t resultSize = 0;
    auto result = TDataPtr(ApplyPatch(nullptr, 0, reinterpret_cast<const ui8*>(base), strlen(base), patch12.get(), patch12Size, derived1Size, &resultSize));

    EXPECT_TRUE(result.get() != nullptr && resultSize);
}

TEST(XDeltaMergePatches, EmptyTarget)
{
    const char* base = "0987654321";
    const char* derived = "";
    size_t derivedSize = strlen(derived);
    size_t patchSize = 0;
    auto patch = TDataPtr(ComputePatch(nullptr, reinterpret_cast<const ui8*>(base), strlen(base), reinterpret_cast<const ui8*>(derived), derivedSize, &patchSize));

    EXPECT_TRUE(patch.get() != nullptr && patchSize);

    size_t resultSize = 0;
    auto result = TDataPtr(ApplyPatch(nullptr, 0, reinterpret_cast<const ui8*>(base), strlen(base), patch.get(), patchSize, derivedSize, &resultSize));

    EXPECT_TRUE(result.get() != nullptr && resultSize == 0);
}

TEST(XDeltaMergePatches, SimpleMerge)
{
    const char* base = "1234567890";
    const char* derived1 = "0987654321";
    const char* derived2 = "666666555554444333221";
    size_t derived2Size = strlen(derived2);
    size_t patch12Size = 0;
    auto patch12 = TDataPtr(ComputePatch(nullptr, reinterpret_cast<const ui8*>(base), strlen(base), reinterpret_cast<const ui8*>(derived1), strlen(derived1), &patch12Size));
    size_t patch23Size = 0;
    auto patch23 = TDataPtr(ComputePatch(nullptr, reinterpret_cast<const ui8*>(derived1), strlen(derived1), reinterpret_cast<const ui8*>(derived2), derived2Size, &patch23Size));

    size_t patch13Size = 0;
    auto patch13 = TDataPtr(MergePatches(nullptr, 0, patch12.get(), patch12Size, patch23.get(), patch23Size, &patch13Size));

    size_t resultSize = 0;
    auto result = TDataPtr(ApplyPatch(nullptr, 0, reinterpret_cast<const ui8*>(base), strlen(base), patch13.get(), patch13Size, derived2Size, &resultSize));

    EXPECT_TRUE(result.get() != nullptr && resultSize);
    EXPECT_TRUE(strlen(derived2) == resultSize);
    EXPECT_EQ(0, memcmp(result.get(), derived2, resultSize));
}

TEST(XDeltaMergePatches, Merge)
{
    const char* a = "";
    const char* b = "123456";
    const char* c = "7890";
    const char* d = "0987654321";
    const char* e = "827476";
    const char* f = "111111";
    size_t patchAbSize;
    auto patchAb = TDataPtr(ComputePatch(nullptr, (const ui8*)a, strlen(a), (const ui8*)b, strlen(b), &patchAbSize));

    size_t patchBcSize;
    auto patchBc = TDataPtr(ComputePatch(nullptr, (const ui8*)b, strlen(b), (const ui8*)c, strlen(c), &patchBcSize));

    size_t patchCdSize;
    auto patchCd = TDataPtr(ComputePatch(nullptr, (const ui8*)c, strlen(c), (const ui8*)d, strlen(d), &patchCdSize));

    size_t patchDeSize;
    auto patchDe = TDataPtr(ComputePatch(nullptr, (const ui8*)d, strlen(d), (const ui8*)e, strlen(e), &patchDeSize));

    size_t patchEfSize;
    auto patchEf = TDataPtr(ComputePatch(nullptr, (const ui8*)e, strlen(e), (const ui8*)f, strlen(f), &patchEfSize));

    size_t mergedPatchAcSize = 0;
    auto mergedPatchAc = TDataPtr(MergePatches(nullptr, 0, patchAb.get(), patchAbSize, patchBc.get(), patchBcSize, &mergedPatchAcSize));

    size_t mergedPatchCeSize = 0;
    auto mergedPatchCe = TDataPtr(MergePatches(nullptr, 0, patchCd.get(), patchCdSize, patchDe.get(), patchDeSize, &mergedPatchCeSize));

    size_t mergedPatchAeSize = 0;
    auto mergedPatchAe = TDataPtr(MergePatches(nullptr, 0, mergedPatchAc.get(), mergedPatchAcSize, mergedPatchCe.get(), mergedPatchCeSize, &mergedPatchAeSize));

    size_t mergedPatchAfSize = 0;
    auto mergedPatchAf = TDataPtr(MergePatches(nullptr, 0, mergedPatchAe.get(), mergedPatchAeSize, patchEf.get(), patchEfSize, &mergedPatchAfSize));

    size_t resultSize = 0;
    auto result = TDataPtr(ApplyPatch(nullptr, 0, (const ui8*)a, strlen(a), mergedPatchAf.get(), mergedPatchAfSize, strlen(f), &resultSize));

    EXPECT_TRUE(resultSize == strlen(f));
    EXPECT_EQ(0, memcmp(result.get(), f, resultSize));
}
