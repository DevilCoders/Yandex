#include "types.h"

#include <util/stream/output.h>

template <>
void Out<NDoom::NItemStorage::TChunkId>(IOutputStream& os, const NDoom::NItemStorage::TChunkId& chunkId) {
    os << "{item: " << static_cast<ui32>(chunkId.ItemType) << ", id: " << chunkId.Id << '}';
}

template <>
void Out<NDoom::NItemStorage::TLumpId>(IOutputStream& os, const NDoom::NItemStorage::TLumpId& lumpId) {
    os << "{item_" << static_cast<ui32>(lumpId.ItemType) << '.' << lumpId.LumpName << '}';
}

template <>
void Out<NDoom::NItemStorage::TItemId>(IOutputStream& os, const NDoom::NItemStorage::TItemId& itemId) {
    os << "{item_" << static_cast<ui32>(itemId.ItemType) << '.' << itemId.ItemKey << '}';
}
