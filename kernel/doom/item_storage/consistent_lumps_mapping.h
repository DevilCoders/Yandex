#pragma once

#include "types.h"

#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

#include <cstddef>

namespace NDoom::NItemStorage {

class TConsistentItemLumpsMapping {
public:
    void RegisterItemLumps(TConstArrayRef<TLumpId> lumps, TArrayRef<size_t> mapping);
    void MapItemLumps(TConstArrayRef<TLumpId> lumps, TArrayRef<size_t> mapping) const;

    TVector<TLumpId> GetLumps() const;
    TConstArrayRef<TLumpId> GetLumpsUnsafe() const;

private:
    THashMap<TLumpId, size_t> Mapping_;
    TVector<TLumpId> Lumps_;
};

} // namespace NDoom::NItemStorage
