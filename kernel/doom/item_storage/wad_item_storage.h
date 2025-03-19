#pragma once

#include "item_storage.h"

#include <util/generic/array_ref.h>
#include <util/folder/path.h>

#include <array>

namespace NDoom::NItemStorage {

enum class EWadOpenMode {
    Mapped,
    DirectAio,
};

TItemStorageBackend MakeWadItemStorageBackend(TItemType item, const TFsPath& path, bool lockMemory = false, EWadOpenMode mode = EWadOpenMode::Mapped);
TItemStorageBackend MakeChunkedWadItemStorageBackend(TItemType item, const TFsPath& prefix, bool lockMemory = false, EWadOpenMode mode = EWadOpenMode::Mapped);

} // namespace NDoom::NItemStorage
