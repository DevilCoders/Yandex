#include "generate.h"

#include <util/generic/array_ref.h>
#include <util/generic/guid.h>
#include <util/generic/xrange.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>

namespace NDoom::NItemStorage::NTest {

ui64 Gen64(TReallyFastRng32& rng) {
    return (ui64{rng()} << 32) | rng();
}

TItemKey GenItemKey(TReallyFastRng32& rng) {
    return TItemKey{Gen64(rng), Gen64(rng)};
}

TItemId GenItemId(TItemType type, TReallyFastRng32& rng) {
    return TItemId{
        .ItemType = type,
        .ItemKey = GenItemKey(rng),
    };
}

TString RandomString(size_t length, ui32 seed) {
    TReallyFastRng32 rng{seed};
    TString result(length, 'a');
    for (char& c : result) {
        constexpr TStringBuf letters{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"};
        c = letters[rng() % letters.size()];
    }
    return result;
}

TLumps GenerateRandomLumpsSubset(TConstArrayRef<TString> names, TReallyFastRng32& rng) {
    Y_ENSURE(names.size() < 32);
    const ui32 gen = rng();
    ui32 mask = gen % (1 << names.size());
    TLumps lumps;
    for (ui32 i = 1, j = 0; i <= mask; i <<= 1, ++j) {
        if (!(mask & i)) {
            continue;
        }

        ui32 seed = rng();
        lumps[names[j]] = RandomString(rng.Uniform(0, 1000), seed);
    }

    return lumps;
}

TVector<TString> GenerateRandomLumpsListSubset(TConstArrayRef<TString> names, TReallyFastRng32& rng) {
    TVector<TString> lumps;
    TLumps map = GenerateRandomLumpsSubset(names, rng);
    for (auto&& [lump, data] : map) {
        lumps.push_back(lump);
    }

    return lumps;
}

TChunk GenerateItemIndexChunk(TConstArrayRef<TString> chunkGlobalLumps, TConstArrayRef<TString> allItemLumps, ui32 numItems, TReallyFastRng32& rng) {
    TVector<TString> itemLumps = GenerateRandomLumpsListSubset(allItemLumps, rng);

    TChunk chunk;
    chunk.ChunkGlobalLumps = GenerateRandomLumpsSubset(chunkGlobalLumps, rng);
    chunk.ItemLumpsList.assign(itemLumps.begin(), itemLumps.end());
    chunk.ChunkGlobalLumpsList.assign(chunkGlobalLumps.begin(), chunkGlobalLumps.end());
    chunk.Uuid = CreateGuidAsString();

    for (ui32 item : xrange(numItems)) {
        Y_UNUSED(item);
        TLumps lumps = GenerateRandomLumpsSubset(itemLumps, rng);

        auto gen = [&rng] {
            return (ui64{rng()} << 32) | rng();
        };
        TItemKey id{gen(), gen()};

        chunk.ItemLumps[id] = std::move(lumps);
        chunk.ItemsList.push_back(id);
    }

    return chunk;
}

TVector<TString> GenerateLumpsList(ui32 numLumps, TReallyFastRng32& rng) {
    TVector<TString> res(numLumps);
    for (TString& s : res) {
        size_t len = rng.Uniform(10, 30);
        ui32 seed = rng();
        s = RandomString(len, seed);
    }
    return res;
}

TItemIndex GenerateItemIndex(const TItemIndexParams& params, TReallyFastRng32& rng) {
    TVector<TString> globalLumps = GenerateLumpsList(params.GlobalLumps, rng);
    TVector<TString> chunkGlobalLumps = GenerateLumpsList(params.ChunkLocalLumps, rng);
    TVector<TString> itemLumps = GenerateLumpsList(params.ItemLumps, rng);

    TItemIndex index;
    index.ItemType = params.ItemType;
    for (TStringBuf lump : globalLumps) {
        index.GlobalLumps[lump] = RandomString(rng.Uniform(0, 1000), rng());
    }
    index.GlobalLumpsList.assign(globalLumps.begin(), globalLumps.end());

    for (ui32 i : xrange(params.NumChunks)) {
        ShuffleRange(chunkGlobalLumps, rng);
        ShuffleRange(itemLumps, rng);
        index.Chunks.push_back(GenerateItemIndexChunk(chunkGlobalLumps, itemLumps, params.NumItems, rng));
        index.Chunks.back().Id = TChunkId{
            .ItemType = params.ItemType,
            .Id = i
        };
    }

    return index;
}

TIndexData GenerateIndex(const TIndexParams& params) {
    TReallyFastRng32 rng{params.Seed};

    TIndexData data;
    for (const TItemIndexParams& params : params.Items) {
        data.Items.push_back(GenerateItemIndex(params, rng));
    }

    return data;
}

} // namespace NDoom::NItemStorage::NTest
