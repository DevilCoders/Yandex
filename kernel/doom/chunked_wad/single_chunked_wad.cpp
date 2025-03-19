#include "single_chunked_wad.h"

#include <util/generic/algorithm.h>

namespace NDoom {


TSingleChunkedWad::TSingleChunkedWad(THolder<IWad> wad)
    : LocalWad_(std::move(wad))
{
    Wad_ = LocalWad_.Get();
}

TSingleChunkedWad::TSingleChunkedWad(const IWad* wad)
{
    Wad_ = wad;
}

TSingleChunkedWad::TSingleChunkedWad(THolder<TMegaWad> wad) {
    InitStub(wad.Get());
    Wad_ = wad.Get();
    LocalWad_ = std::move(wad);
}

TSingleChunkedWad::TSingleChunkedWad(const TMegaWad* wad) {
    InitStub(wad);
    Wad_ = wad;
}

void TSingleChunkedWad::InitStub(const TMegaWad* wad) {
    LoaderStub_.ConstructInPlace();
    auto lumps = wad->DocLumps();
    for (size_t i = 0; i < lumps.size(); ++i) {
        LoaderStub_->push_back(i);
    }
}

ui32 TSingleChunkedWad::Size() const {
    return Wad_->Size();
}

TVector<TWadLumpId> TSingleChunkedWad::GlobalLumps() const {
    return Wad_->GlobalLumps();
}

bool TSingleChunkedWad::HasGlobalLump(TWadLumpId type) const {
    return Wad_->HasGlobalLump(type);
}

TVector<TWadLumpId> TSingleChunkedWad::DocLumps() const {
    return Wad_->DocLumps();
}

TBlob TSingleChunkedWad::LoadGlobalLump(TWadLumpId type) const {
    return Wad_->LoadGlobalLump(type);
}

void TSingleChunkedWad::MapDocLumps(const TArrayRef<const TWadLumpId>& types, TArrayRef<size_t> mapping) const {
    Wad_->MapDocLumps(types, mapping);
}

TBlob TSingleChunkedWad::LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const {
    return Wad_->LoadDocLumps(docId, mapping, regions);
}

TVector<TBlob> TSingleChunkedWad::LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const {
    return Wad_->LoadDocLumps(docIds, mapping, regions);
}

TVector<TWadLumpId> TSingleChunkedWad::ChunkGlobalLumps(ui32 chunk) const {
    Y_ENSURE(chunk < 1);
    return Wad_->GlobalLumps();
}

bool TSingleChunkedWad::HasChunkGlobalLump(ui32 chunk, TWadLumpId type) const {
    Y_ENSURE(chunk < 1);
    return Wad_->HasGlobalLump(type);
}

TBlob TSingleChunkedWad::LoadChunkGlobalLump(ui32 chunk, TWadLumpId type) const {
    Y_ENSURE(chunk < 1);
    return Wad_->LoadGlobalLump(type);
}

ui32 TSingleChunkedWad::Chunks() const {
    return 1;
}

ui32 TSingleChunkedWad::DocChunk(ui32) const {
    return 0;
}

const IDocLumpMapper& TSingleChunkedWad::Mapper() const {
    Y_VERIFY(LoaderStub_);
    return *Wad_;
}

TUnanswersStats TSingleChunkedWad::Fetch(TConstArrayRef<ui32> ids, std::function<void(size_t, TChunkedWadDocLumpLoader*)> cb) const {
    Y_VERIFY(LoaderStub_);
    TVector<TArrayRef<TArrayRef<const char>>> regionStubs{ids.size()};
    auto blobs = Wad_->LoadDocLumps(ids, {}, regionStubs);
    for (size_t i = 0; i < blobs.size(); ++i) {
        TChunkedWadDocLumpLoader loader{std::move(blobs[i]), 0, *LoaderStub_, LoaderStub_->size()};
        cb(i, &loader);
    }
    return TUnanswersStats{static_cast<ui32>(ids.size()), 0};
}

} // namespace NDoom
