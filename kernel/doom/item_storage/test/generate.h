#pragma once

#include "index.h"

#include <kernel/doom/item_storage/types.h>

#include <util/random/fast.h>

namespace NDoom::NItemStorage::NTest {

#define DECLARE_FIELD(Type, Name, Default) \
Type Name = Default; \
auto& Set##Name(Type value) { \
    Name = std::move(value); \
    return *this; \
}

#define DECLARE_REPEATED_FIELD(Type, Name) \
TVector<Type> Name; \
auto& Add##Name(Type value) { \
    Name.push_back(std::move(value)); \
    return *this; \
}

struct TItemIndexParams {
    DECLARE_FIELD(ui8, ItemType, 0);
    DECLARE_FIELD(ui32, NumItems, 0);
    DECLARE_FIELD(ui32, NumChunks, 0);

    DECLARE_FIELD(ui32, GlobalLumps, 1);
    DECLARE_FIELD(ui32, ChunkLocalLumps, 1);
    DECLARE_FIELD(ui32, ItemLumps, 1);
};

struct TIndexParams {
    DECLARE_REPEATED_FIELD(TItemIndexParams, Items);
    DECLARE_FIELD(ui32, Seed, 02720);
};

#undef DECLARE_FIELD
#undef DECLARE_REPEATED_FIELD

TIndexData GenerateIndex(const TIndexParams& params);
TString RandomString(size_t length, ui32 seed);
TItemId GenItemId(TItemType type, TReallyFastRng32& rng);

} // namespace NDoom::NItemStorage::NTest
