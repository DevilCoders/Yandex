#pragma once

#include "chunked_wad.h"
#include "mapper.h"

#include <kernel/doom/doc_lump_fetcher/doc_lump_fetcher.h>
#include <kernel/doom/wad/mega_wad.h>

#include <library/cpp/offroad/flat/flat_searcher.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>


namespace NDoom {


class TSingleChunkedWad: public IChunkedWad, public IDocLumpFetcher<TChunkedWadDocLumpLoader> {
public:
    TSingleChunkedWad(THolder<IWad> wad);
    TSingleChunkedWad(const IWad* wad);

    TSingleChunkedWad(THolder<TMegaWad> wad);

    TSingleChunkedWad(const TMegaWad* wad);

    ui32 Size() const override;

    TVector<TWadLumpId> GlobalLumps() const override;

    bool HasGlobalLump(TWadLumpId id) const override;

    TVector<TWadLumpId> DocLumps() const override;

    TBlob LoadGlobalLump(TWadLumpId id) const override;

    void MapDocLumps(const TArrayRef<const TWadLumpId>& ids, TArrayRef<size_t> mapping) const override;

    TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const override;

    TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const override;

    TVector<TWadLumpId> ChunkGlobalLumps(ui32 chunk) const override;

    bool HasChunkGlobalLump(ui32 chunk, TWadLumpId type) const override;

    TBlob LoadChunkGlobalLump(ui32 chunk, TWadLumpId type) const override;

    ui32 Chunks() const override;

    ui32 DocChunk(ui32 docId) const override;

    const IDocLumpMapper& Mapper() const override;

    TUnanswersStats Fetch(TConstArrayRef<ui32> ids, std::function<void(size_t, TChunkedWadDocLumpLoader*)> cb) const override;

private:
    void InitStub(const TMegaWad* wad);

private:
    THolder<IWad> LocalWad_;
    const IWad* Wad_ = nullptr;

    TMaybe<TVector<size_t>> LoaderStub_;
};


} // namespace NDoom
