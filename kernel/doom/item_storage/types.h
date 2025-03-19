#pragma once

#include "item_key.h"

#include <util/system/types.h>
#include <util/generic/string.h>

#include <compare>

namespace NDoom::NItemStorage {

using TItemType = ui8;

inline constexpr size_t NumItemTypes() {
    return static_cast<size_t>(Max<TItemType>()) + 1;
}

using TLumpName = TString;
using TLumpNameRef = TStringBuf;

struct TLumpId {
    TItemType ItemType = 0;
    TLumpName LumpName;

    inline auto operator<=>(const TLumpId& rhs) const = default;
};

struct TChunkId {
    TItemType ItemType = 0;
    ui32 Id = 0;

    inline auto operator<=>(const TChunkId& rhs) const = default;
};

struct TItemId {
    TItemType ItemType = 0;
    TItemKey ItemKey = 0;

    inline auto operator<=>(const TItemId& rhs) const = default;
};

} // namespace NDoom::NItemStorage

template <>
struct THash<NDoom::NItemStorage::TLumpId> {
    size_t operator()(const NDoom::NItemStorage::TLumpId& id) const {
        return CombineHashes(THash<NDoom::NItemStorage::TItemType>{}(id.ItemType), THash<TStringBuf>{}(id.LumpName));
    }
};

template <>
struct THash<NDoom::NItemStorage::TChunkId> {
    size_t operator()(const NDoom::NItemStorage::TChunkId& id) const {
        return CombineHashes(THash<NDoom::NItemStorage::TItemType>{}(id.ItemType), THash<ui32>{}(id.Id));
    }
};

template <>
struct THash<NDoom::NItemStorage::TItemId> {
    size_t operator()(const NDoom::NItemStorage::TItemId& id) const {
        return CombineHashes(THash<NDoom::NItemStorage::TItemType>{}(id.ItemType), THash<ui128>{}(id.ItemKey));
    }
};
