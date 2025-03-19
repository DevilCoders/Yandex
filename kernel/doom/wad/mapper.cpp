#include "mapper.h"

namespace NDoom {

void IDocLumpMapper::MapDocLumps(TConstArrayRef<TStringBuf> /* names */, TArrayRef<size_t> /* mapping */) const {
    Y_ENSURE(false, "Unimplemented");
}

void IDocLumpMapper::MapDocLumps(TConstArrayRef<TString> /* names */, TArrayRef<size_t> /* mapping */) const {
    Y_ENSURE(false, "Unimplemented");
}

TVector<TStringBuf> IDocLumpMapper::DocLumpsNames() const {
    Y_ENSURE(false, "Unimplemented");
}

} // namespace NDoom
