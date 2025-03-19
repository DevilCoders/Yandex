#pragma once

#include "index.h"

#include <kernel/doom/item_storage/item_storage.h>

namespace NDoom::NItemStorage::NTest {

    void TestGlobalLumps(const TIndexData& data, const THolder<IItemStorage>& storage);

    void TestChunkGlobalLumps(const TIndexData& data, const THolder<IItemStorage>& storage);

    void TestItemLumps(const TIndexData& data, const THolder<IItemStorage>& storage);

    void TestContainsItemCheck(const TIndexData& data, const THolder<IItemStorage>& storage);

} // namespace NDoom::NItemStorage::NTest
