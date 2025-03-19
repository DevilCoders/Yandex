#pragma once

#include <kernel/doom/item_storage/types.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NDoom::NItemStorage::NTest {

using TLumps = THashMap<TLumpName, TString>;

struct TChunk {
    TChunkId Id;
    TString Uuid;

    TLumps ChunkGlobalLumps;
    TVector<TLumpName> ChunkGlobalLumpsList;

    THashMap<TItemKey, TLumps> ItemLumps;
    TVector<TLumpName> ItemLumpsList;

    TVector<TItemKey> ItemsList;
};

struct TItemIndex {
    TItemType ItemType;

    TLumps GlobalLumps;
    TVector<TString> GlobalLumpsList;

    TVector<TChunk> Chunks;
};

struct TIndexData {
    TVector<TItemIndex> Items;
};

}
