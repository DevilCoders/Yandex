#pragma once

#include <kernel/doom/doc_lump_fetcher/doc_lump_fetcher.h>

#include <kernel/doom/wad/mega_wad_info.h>
#include <kernel/doom/wad/mega_wad_common.h>
#include <kernel/doom/wad/wad_lump_id.h>

#include <kernel/doom/chunked_wad/chunked_wad.h>

#include <library/cpp/threading/thread_local/thread_local.h>

#include <util/memory/blob.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>


namespace NDoom {

template<typename BaseMapper = IDocLumpMapper, typename BaseLoader = IDocLumpLoader>
class TWadStub : public IChunkedWad {
public:
    TWadStub(NDoom::IChunkedWad& global, BaseMapper* mapper)
        : Global_(global)
        , Mapper_(mapper)
    {
    }

    void Put(size_t id, BaseLoader* loader) {
        TLocalData& data = Data_.GetRef();
        data.Loader = loader;
        data.Id = id;
    }

    TVector<TWadLumpId> ChunkGlobalLumps(ui32 chunk) const override {
        return Global_.ChunkGlobalLumps(chunk);
    }

    bool HasChunkGlobalLump(ui32 chunk, TWadLumpId type) const override {
        return Global_.HasChunkGlobalLump(chunk, type);
    }

    TBlob LoadChunkGlobalLump(ui32 chunk, TWadLumpId type) const override {
        return Global_.LoadChunkGlobalLump(chunk, type);
    }

    ui32 Chunks() const override {
        return Global_.Chunks();
    }

    ui32 DocChunk(ui32 docId) const override {
        return Global_.DocChunk(docId);
    }

    ui32 Size() const override {
        return Global_.Size();
    }

    TVector<TWadLumpId> GlobalLumps() const override {
        return Global_.GlobalLumps();
    }

    bool HasGlobalLump(TWadLumpId id) const override {
        return Global_.HasGlobalLump(id);
    }

    TBlob LoadGlobalLump(TWadLumpId id) const override {
        return Global_.LoadGlobalLump(id);
    }

    TVector<TWadLumpId> DocLumps() const override {
        return Mapper_->DocLumps();
    }

    void MapDocLumps(const TArrayRef<const TWadLumpId>& ids, TArrayRef<size_t> mapping) const override {
        Mapper_->MapDocLumps(ids, mapping);
    }

    TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const override {
        TLocalData& data = Data_.GetRef();
        Y_ENSURE(docId == data.Id);
        data.Loader->LoadDocRegions(mapping, regions);
        return data.Loader->DataHolder();
    }

private:
    struct TLocalData {
        ui32 Id = Max<ui32>();
        BaseLoader* Loader = nullptr;
    };

private:
    IChunkedWad& Global_;

    NThreading::TThreadLocalValue<TLocalData> Data_;
    BaseMapper* Mapper_;
};


template<typename BaseFetcher = IDocLumpFetcher<>, typename WadStub = TWadStub<>>
class TWadStubFetcher {
public:
    TWadStubFetcher(BaseFetcher& fetcher, TConstArrayRef<WadStub> stubs)
        : Stubs_(stubs)
        , Fetcher_(fetcher)
    {
    }

    template<typename Callback>
    void Fetch(TConstArrayRef<ui32> ids, Callback cb) {
        Fetcher_.Fetch(
            ids,
            [&](size_t id, const typename BaseFetcher::Loader* loader) {
                for (size_t i = 0; i < Stubs_.size(); ++i) {
                    Stubs_[i].Put(loader);
                }
                cb(id);
            });
    }

private:
    TVector<WadStub> Stubs_;
    BaseFetcher& Fetcher_;
};

} // namespace NDoom
