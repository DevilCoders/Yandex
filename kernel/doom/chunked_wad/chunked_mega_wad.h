#pragma once

#include "chunked_wad.h"

#include "doc_chunk_mapping.h"

#include "mapper.h"

#include <kernel/doom/blob_storage/chunked_blob_storage.h>
#include <kernel/doom/doc_lump_fetcher/doc_lump_fetcher.h>

#include <kernel/doom/wad/mega_wad_common.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>


namespace NDoom {

// Loaders returned from Fetch are not compatible with MapDocLumps()
class TChunkedMegaWad: public IChunkedWad, public IDocLumpFetcher<TChunkedWadDocLumpLoader> {
public:
    TChunkedMegaWad(
        const TString& prefix,
        bool lockMemory);

    TChunkedMegaWad(
        THolder<IWad>&& global,
        THolder<IWad>&& docChunkMapping,
        const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
        const IChunkedBlobStorage* docLumpChunkedBlobStorage,
        TVector<TMegaWadCommon>&& chunkMegaWadInfos);

    TChunkedMegaWad(
        THolder<IWad>&& global,
        THolder<IWad>&& docChunkMapping,
        const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
        const IChunkedBlobStorage* docLumpChunkedBlobStorage);

    TChunkedMegaWad(
        THolder<IWad>&& global,
        TDocChunkMappingSearcher* docChunkMappingSearcher,
        const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
        const IChunkedBlobStorage* docLumpChunkedBlobStorage,
        TVector<TMegaWadCommon>&& chunkMegaWadInfos);

    TChunkedMegaWad(
        THolder<IWad>&& global,
        TDocChunkMappingSearcher* docChunkMappingSearcher,
        const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
        const IChunkedBlobStorage* docLumpChunkedBlobStorage);

    ui32 Size() const override;

    TVector<TWadLumpId> GlobalLumps() const override;

    bool HasGlobalLump(TWadLumpId id) const override;

    TVector<TWadLumpId> DocLumps() const override;

    TBlob LoadGlobalLump(TWadLumpId id) const override;

    void MapDocLumps(const TArrayRef<const TWadLumpId>& ids, TArrayRef<size_t> mapping) const override;

    TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const override;

    void LoadDocLumps(
        const TArrayRef<const ui32>& docIds,
        const TArrayRef<const size_t>& mapping,
        std::function<void(size_t, TMaybe<TDocLumpData>&&)> callback) const override;

    TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const override;

    TVector<TWadLumpId> ChunkGlobalLumps(ui32 chunk) const override;

    bool HasChunkGlobalLump(ui32 chunk, TWadLumpId type) const override;

    TBlob LoadChunkGlobalLump(ui32 chunk, TWadLumpId type) const override;

    ui32 Chunks() const override;

    ui32 DocChunk(ui32 docId) const override;

    TDocChunkMapping DocMapping(ui32 docId) const;

    TUnanswersStats Fetch(TConstArrayRef<ui32> ids, std::function<void(size_t, TChunkedWadDocLumpLoader*)> cb) const override;

    const IDocLumpMapper& Mapper() const override {
        return *FetchMapper_;
    }

private:
    enum {
        MaxDocLumps = 16,
        MaxDocs = 16,
    };

    void ResetChunkMegaWadInfos();

    void Validate();

    void ResetDocLumps();

    template <class Region>
    TBlob LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const;

    template <class Region>
    TVector<TBlob> LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const;

    template <class Region, class Loader>
    auto LoadFromChunk(ui32 chunk, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions, Loader&& loader) const {
        Y_ENSURE(mapping.size() == regions.size());
        Y_ENSURE(mapping.size() <= MaxDocLumps);

        using TReturnType = decltype(loader(mapping, regions));

        if (Y_LIKELY(!NeedRemapChunkDocLumps_[chunk])) {
            return loader(mapping, regions);
        }

        const TVector<size_t>& remapping = ChunkDocLumpsRemapping_[chunk];

        std::array<size_t, MaxDocLumps> chunkMapping;

        size_t size = 0;

        for (size_t current : mapping) {
            Y_ENSURE(current < remapping.size());
            current = remapping[current];
            if (current != Max<size_t>()) {
                chunkMapping[size++] = current;
            }
        }

        if (Y_LIKELY(size == mapping.size())) {
            return loader(
                TArrayRef<const size_t>(chunkMapping.data(), size),
                regions);
        }

        if (Y_UNLIKELY(size == 0)) {
            for (size_t i = 0; i < mapping.size(); ++i) {
                regions[i] = Region();
            }
            return TReturnType();
        }

        std::array<Region, MaxDocLumps> chunkRegions;

        TReturnType result = loader(
            TArrayRef<const size_t>(chunkMapping.data(), size),
            TArrayRef<Region>(chunkRegions.data(), size));

        size_t ptr = 0;
        for (size_t i = 0; i < mapping.size(); ++i) {
            if (remapping[mapping[i]] == Max<size_t>()) {
                regions[i] = Region();
            } else {
                regions[i] = std::move(chunkRegions[ptr++]);
            }
        }

        return result;
    }


    THolder<IWad> Global_;
    THolder<IWad> DocChunkMappingWad_;
    TMaybe<TDocChunkMappingSearcher> InternalDocChunkMappingSearcher_;
    TDocChunkMappingSearcher* DocChunkMappingSearcher_ = nullptr;



    THolder<IChunkedBlobStorage> LocalChunkedBlobStorage_;
    const IChunkedBlobStorage* GlobalLumpChunkedBlobStorage_ = nullptr;
    const IChunkedBlobStorage* DocLumpChunkedBlobStorage_ = nullptr;
    TVector<TMegaWadCommon> ChunkMegaWadInfos_;

    THashMap<TWadLumpId, size_t> DocLumpIndexByType_;
    TVector<TWadLumpId> AllDocLumps_;
    TVector<bool> NeedRemapChunkDocLumps_;
    TVector<TVector<size_t>> ChunkDocLumpsRemapping_;

    TMaybe<NDoom::TChunkedWadDocLumpMapper> FetchMapper_;
};


} // namespace NDoom
