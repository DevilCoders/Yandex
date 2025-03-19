#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/vector.h>

#include <cstddef>

namespace NDoom::NItemStorage {

class TLumpsSet {
public:
    TLumpsSet() = default;
    TLumpsSet(TVector<size_t> mapping);

    TConstArrayRef<size_t> GetMapping() const;
    size_t NumLumps() const;

private:
    TVector<size_t> Mapping_;
};


} // namespace NDoom::NItemStorage
