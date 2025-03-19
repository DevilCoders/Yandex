#pragma once

#include "single_chunked_wad.h"


namespace NDoom {


class TOnlyGlobalChunkedWad: public TSingleChunkedWad {
public:
    using TSingleChunkedWad::TSingleChunkedWad;

    TOnlyGlobalChunkedWad(THolder<TMegaWad> wad, ui32 size)
        : TSingleChunkedWad(std::move(wad))
    {
        Size_ = size;
    }

    TBlob LoadDocLumps(ui32, const TArrayRef<const size_t>&, TArrayRef<TArrayRef<const char>>) const override {
        SEARCH_ERROR << "LoadDocLumps in TOnlyGlobalChunkedWad is not supported.";
        throw yexception() << "LoadDocLumps in TOnlyGlobalChunkedWad is not supported.";
    }

    TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>&, const TArrayRef<const size_t>&, TArrayRef<TArrayRef<TArrayRef<const char>>>) const override {
        SEARCH_ERROR << "LoadDocLumps in TOnlyGlobalChunkedWad is not supported.";
        throw yexception() << "LoadDocLumps in TOnlyGlobalChunkedWad is not supported.";
    }

    ui32 Size() const override {
        if (Size_) {
            return Size_.GetRef();
        }
        return TSingleChunkedWad::Size();
    }

private:
    TMaybe<ui32> Size_;
};


} // namespace NDoom
