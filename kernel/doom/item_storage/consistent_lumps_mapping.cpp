#include "consistent_lumps_mapping.h"

#include <library/cpp/iterator/enumerate.h>

namespace NDoom::NItemStorage {

void TConsistentItemLumpsMapping::RegisterItemLumps(TConstArrayRef<TLumpId> lumps, TArrayRef<size_t> mapping) {
    for (const auto&& [i, lump] : Enumerate(lumps)) {
        if (const size_t* ptr = Mapping_.FindPtr(lump)) {
            mapping[i] = *ptr;
        } else {
            const size_t index = Mapping_.size();
            Mapping_.emplace(lump, index);
            Lumps_.push_back(lump);
            mapping[i] = index;
            Y_ASSERT(Lumps_.at(Mapping_.at(lump)) == lump);
        }
    }
}

void TConsistentItemLumpsMapping::MapItemLumps(TConstArrayRef<TLumpId> lumps, TArrayRef<size_t> mapping) const {
    for (const auto&& [i, lump] : Enumerate(lumps)) {
        if (const size_t* ptr = Mapping_.FindPtr(lump)) {
            mapping[i] = *ptr;
        } else {
            mapping[i] = Max<size_t>();
        }
    }
}

TVector<TLumpId> TConsistentItemLumpsMapping::GetLumps() const {
    return Lumps_;
}

TConstArrayRef<TLumpId> TConsistentItemLumpsMapping::GetLumpsUnsafe() const {
    return Lumps_;
}

} // namespace NDoom::NItemStorage
