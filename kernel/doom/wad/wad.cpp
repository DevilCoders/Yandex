#include <array>

#include <util/string/cast.h>
#include <util/generic/buffer.h>

#include "wad.h"
#include "mega_wad.h"

namespace NDoom {


struct TWadHolder {
    TBuffer Buffer;
};

template<class Base>
struct THoldingWad : public Base, public TWadHolder {
    using Base::Base;
};

THolder<IWad> IWad::Open(const TString& path, bool lockMemory) {
    return MakeHolder<THoldingWad<TMegaWad>>(path, lockMemory);
}

THolder<IWad> IWad::Open(const TArrayRef<const char>& source) {
    return MakeHolder<THoldingWad<TMegaWad>>(source);
}

THolder<IWad> IWad::Open(TBuffer&& buffer) {
    THolder<IWad> result = IWad::Open(TArrayRef<const char>(buffer.data(), buffer.size()));

    /* This is ugly, but this way the code actually gets cleaner. */
    TWadHolder* holder = dynamic_cast<TWadHolder*>(result.Get());
    Y_VERIFY(holder);

    holder->Buffer = std::move(buffer);

    return result;
}

THolder<IWad> IWad::Open(const TBlob& blob) {
    return MakeHolder<THoldingWad<TMegaWad>>(blob);
}

TVector<TString> IWad::GlobalLumpsNames() const {
    Y_ENSURE(false, "Unimplemented");
}

bool IWad::HasGlobalLump(TStringBuf /* id */) const {
    Y_ENSURE(false, "Unimplemented");
}

TBlob IWad::LoadGlobalLump(TStringBuf /* id */) const {
    Y_ENSURE(false, "Unimplemented");
}

void IWad::LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, std::function<void(size_t, TMaybe<TDocLumpData>&&)> callback) const {
    TVector<TArrayRef<const char>> regions(docIds.size() * mapping.size());
    TVector<TArrayRef<TArrayRef<const char>>> refs(docIds.size());
    for (size_t i = 0; i < docIds.size(); ++i) {
        refs[i] = TArrayRef<TArrayRef<const char>>(&regions[i * mapping.size()], mapping.size());
    }
    TVector<TBlob> blobs = LoadDocLumps(docIds, mapping, refs);
    for (size_t i = 0; i < refs.size(); ++i) {
        callback(i, TDocLumpData{blobs[i], refs[i]});
    }
}

TVector<TBlob> IWad::LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const {
        TVector<TBlob> blobs(docIds.size());
        for (size_t i = 0; i < docIds.size(); ++i) {
            blobs[i] = LoadDocLumps(docIds[i], mapping, regions[i]);
        }
        return blobs;
    }

} // namespace NDoom
