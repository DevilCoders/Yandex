#include "lumps_set.h"

namespace NDoom::NItemStorage {

TLumpsSet::TLumpsSet(TVector<size_t> mapping)
    : Mapping_{ std::move(mapping) }
{
}

TConstArrayRef<size_t> TLumpsSet::GetMapping() const {
    return Mapping_;
}

size_t TLumpsSet::NumLumps() const {
    return GetMapping().size();
}

} // namespace NDoom::NItemStorage
