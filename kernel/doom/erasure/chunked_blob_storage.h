#pragma once

#include "loc_resolver.h"

#include <kernel/doom/flat_blob_storage/flat_blob_storage.h>
#include <kernel/doom/blob_storage/chunked_blob_storage.h>

namespace NDoom {

class TErasureChunkedBlobStorage: public IChunkedBlobStorage {
public:
    TErasureChunkedBlobStorage() = default;

    TErasureChunkedBlobStorage(TConstArrayRef<TString> resolvers, bool lockMemory) {
        Globals_ = TVector<THolder<IWad>>(resolvers.size());
        for (size_t i = 0; i < resolvers.size(); ++i) {
            Globals_[i] = IWad::Open(resolvers[i], lockMemory);
            Infos_.push_back(LoadMegaWadInfo(Globals_[i]->LoadGlobalLump({EWadIndexType::ErasurePartLocations, EWadLumpRole::HitSub})));
            GlobalMapping_.emplace_back();
            Infos_[i].GlobalLumps.clear();

            TVector<TString> chunkGlobalLumps = Globals_[i]->GlobalLumpsNames();
            for (size_t j = 0; j < chunkGlobalLumps.size(); ++j) {
                GlobalMapping_[i][j] = chunkGlobalLumps[j];
                Infos_[i].GlobalLumps[chunkGlobalLumps[j]] = j;
            }
            Infos_[i].FirstDoc = Globals_[i]->GlobalLumps().size();
        }
    }

    ui32 Chunks() const override {
        return Globals_.size();
    }

    ui32 ChunkSize(ui32 chunk) const override {
        return Infos_[chunk].DocCount + Infos_[chunk].GlobalLumps.size();
    }

    TBlob Read(ui32 chunk, ui32 id) const override {
        Y_ENSURE(Infos_[chunk].FirstDoc > id);

        const TString* lumpId = GlobalMapping_[chunk].FindPtr(id);
        Y_ENSURE(lumpId);
        return Globals_[chunk]->LoadGlobalLump(*lumpId);
    }

    TVector<TMegaWadInfo> ChunkInfos() const {
        return Infos_;
    }

protected:
    TVector<THashMap<ui32, TString>> GlobalMapping_;
    TVector<TMegaWadInfo> Infos_;
    TVector<THolder<IWad>> Globals_;
};

template<typename FlatStorage = IFlatBlobStorage>
class TRealErasureChunkedBlobStorage: public TErasureChunkedBlobStorage {
public:
    TRealErasureChunkedBlobStorage(TConstArrayRef<TString> partResolvers, TConstArrayRef<TString> offsetResolvers, bool lockMemory, THolder<FlatStorage> storage)
        : TErasureChunkedBlobStorage(offsetResolvers, lockMemory)
        , Storage_(std::move(storage))
        , Resolvers_(offsetResolvers.size())
    {
        Y_ENSURE(Storage_);

        for (ui32 i = 0; i < offsetResolvers.size(); ++i) {
            PartResolversHolders_.push_back(IWad::Open(partResolvers[i]));
            Resolvers_[i].Reset(Globals_[i].Get(), 0, PartResolversHolders_[i].Get());
        }
    }

    TBlob Read(ui32 chunk, ui32 id) const override {
        return Read(TArrayRef<const ui32>{chunk}, TArrayRef<const ui32>{id})[0];
    }

    TVector<TBlob> Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids) const override
    {
        Y_ASSERT(chunks.size() == ids.size());

        TVector<ui32> parts(chunks.size());
        TVector<ui64> offsets(chunks.size());
        TVector<ui64> sizes(chunks.size());
        TVector<TBlob> blobs(chunks.size());

        ResolveLocations(chunks, ids, parts, offsets, sizes);

        Storage_->Read(chunks, parts, offsets, sizes, blobs);
        return blobs;
    }

    void Read(
        const TArrayRef<const ui32>& chunks,
        const TArrayRef<const ui32>& ids,
        std::function<void(size_t, TMaybe<TBlob>&&)> callback) const override
    {
        Y_ASSERT(chunks.size() == ids.size());

        TVector<ui32> parts(chunks.size());
        TVector<ui64> offsets(chunks.size());
        TVector<ui64> sizes(chunks.size());

        ResolveLocations(chunks, ids, parts, offsets, sizes);

        Storage_->Read(chunks, parts, offsets, sizes, std::move(callback));
    }

private:
    void ResolveLocations(
        TConstArrayRef<ui32> chunks,
        TConstArrayRef<const ui32> ids,
        TArrayRef<ui32> parts,
        TArrayRef<ui64> offsets,
        TArrayRef<ui64> sizes) const
    {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Y_ENSURE(ids[i] >= Infos_[chunks[i]].FirstDoc);

            auto loc = Resolvers_[chunks[i]].Resolve(ids[i] - Infos_[chunks[i]].FirstDoc);
            Y_ENSURE(loc);

            parts[i] = loc->Part;
            offsets[i] = loc->Offset;
            sizes[i] = loc->Size;
        }
    }

private:
    THolder<FlatStorage> Storage_;
    TVector<THolder<IWad>> PartResolversHolders_;
    TVector<TPartOptimizedErasureLocationResolver> Resolvers_;
};

} // namespace NDoom
