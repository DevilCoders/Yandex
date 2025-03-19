#include "chunked_mega_wad.h"
#include "util/generic/yexception.h"

#include <kernel/doom/blob_storage/chunked_blob_storage_with_cache.h>
#include <kernel/doom/blob_storage/mapped_wad_chunked_blob_storage.h>

#include <kernel/doom/wad/wad_signature.h>

#include <util/generic/algorithm.h>
#include <util/memory/blob.h>
#include <util/string/builder.h>
#include <util/system/fs.h>
#include <util/system/mutex.h>

namespace NDoom {


TChunkedMegaWad::TChunkedMegaWad(
    const TString& prefix,
    bool lockMemory)
    : Global_(IWad::Open(prefix + ".global.wad", lockMemory))
    , InternalDocChunkMappingSearcher_(MakeMaybe<TDocChunkMappingSearcher>(prefix + ".mapping.wad", lockMemory))
    , DocChunkMappingSearcher_(InternalDocChunkMappingSearcher_.Get())
{
    TVector<TString> chunkPaths;
    for (size_t i = 0; ; ++i) {
        TString chunkPath = TStringBuilder() << prefix << "." << i << ".wad";
        if (!NFs::Exists(chunkPath)) {
            break;
        }
        chunkPaths.push_back(std::move(chunkPath));
    }

    LocalChunkedBlobStorage_ = MakeHolder<TMappedWadChunkedBlobStorage>(chunkPaths, lockMemory, ToString(MegaWadSignature));
    GlobalLumpChunkedBlobStorage_ = LocalChunkedBlobStorage_.Get();
    DocLumpChunkedBlobStorage_ = LocalChunkedBlobStorage_.Get();

    ResetChunkMegaWadInfos();

    Validate();
    ResetDocLumps();
}

TChunkedMegaWad::TChunkedMegaWad(
    THolder<IWad>&& global,
    THolder<IWad>&& docChunkMapping,
    const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
    const IChunkedBlobStorage* docLumpChunkedBlobStorage,
    TVector<TMegaWadCommon>&& chunkMegaWadInfos)
    : Global_(std::forward<THolder<IWad>>(global))
    , DocChunkMappingWad_(std::forward<THolder<IWad>>(docChunkMapping))
    , InternalDocChunkMappingSearcher_(MakeMaybe<TDocChunkMappingSearcher>(DocChunkMappingWad_.Get()))
    , DocChunkMappingSearcher_(InternalDocChunkMappingSearcher_.Get())
    , GlobalLumpChunkedBlobStorage_(globalLumpChunkedBlobStorage)
    , DocLumpChunkedBlobStorage_(docLumpChunkedBlobStorage)
    , ChunkMegaWadInfos_(std::forward<TVector<TMegaWadCommon>>(chunkMegaWadInfos))
{
    Validate();
    ResetDocLumps();
}

TChunkedMegaWad::TChunkedMegaWad(
    THolder<IWad>&& global,
    THolder<IWad>&& docChunkMapping,
    const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
    const IChunkedBlobStorage* docLumpChunkedBlobStorage)
    : Global_(std::forward<THolder<IWad>>(global))
    , DocChunkMappingWad_(std::forward<THolder<IWad>>(docChunkMapping))
    , InternalDocChunkMappingSearcher_(MakeMaybe<TDocChunkMappingSearcher>(DocChunkMappingWad_.Get()))
    , DocChunkMappingSearcher_(InternalDocChunkMappingSearcher_.Get())
    , GlobalLumpChunkedBlobStorage_(globalLumpChunkedBlobStorage)
    , DocLumpChunkedBlobStorage_(docLumpChunkedBlobStorage)
{
    ResetChunkMegaWadInfos();

    Validate();
    ResetDocLumps();
}


TChunkedMegaWad::TChunkedMegaWad(
        THolder<IWad>&& global,
        TDocChunkMappingSearcher* docChunkMappingSearcher,
        const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
        const IChunkedBlobStorage* docLumpChunkedBlobStorage,
        TVector<TMegaWadCommon>&& chunkMegaWadInfos)
    : Global_(std::forward<THolder<IWad>>(global))
    , DocChunkMappingWad_(nullptr)
    , DocChunkMappingSearcher_(docChunkMappingSearcher)
    , GlobalLumpChunkedBlobStorage_(globalLumpChunkedBlobStorage)
    , DocLumpChunkedBlobStorage_(docLumpChunkedBlobStorage)
    , ChunkMegaWadInfos_(std::forward<TVector<TMegaWadCommon>>(chunkMegaWadInfos))
{
    Validate();
    ResetDocLumps();
}

TChunkedMegaWad::TChunkedMegaWad(
        THolder<IWad>&& global,
        TDocChunkMappingSearcher* docChunkMappingSearcher,
        const IChunkedBlobStorage* globalLumpChunkedBlobStorage,
        const IChunkedBlobStorage* docLumpChunkedBlobStorage)
    : Global_(std::forward<THolder<IWad>>(global))
    , DocChunkMappingWad_(nullptr)
    , DocChunkMappingSearcher_(docChunkMappingSearcher)
    , GlobalLumpChunkedBlobStorage_(globalLumpChunkedBlobStorage)
    , DocLumpChunkedBlobStorage_(docLumpChunkedBlobStorage)
{
    ResetChunkMegaWadInfos();

    Validate();
    ResetDocLumps();
}

void TChunkedMegaWad::ResetChunkMegaWadInfos() {
    ChunkMegaWadInfos_.resize(GlobalLumpChunkedBlobStorage_->Chunks());
    for (size_t i = 0; i < ChunkMegaWadInfos_.size(); ++i) {
        ChunkMegaWadInfos_[i].Reset(GlobalLumpChunkedBlobStorage_, i);
    }
}

void TChunkedMegaWad::Validate() {
    Y_ENSURE(GlobalLumpChunkedBlobStorage_->Chunks() == DocLumpChunkedBlobStorage_->Chunks());
    Y_ENSURE(ChunkMegaWadInfos_.size() == GlobalLumpChunkedBlobStorage_->Chunks());
    for (size_t i = 0; i < ChunkMegaWadInfos_.size(); ++i) {
        TMegaWadCommon& megaWadInfo = ChunkMegaWadInfos_[i];
        Y_ENSURE(megaWadInfo.Info().FirstDoc + megaWadInfo.Info().DocCount <= DocLumpChunkedBlobStorage_->ChunkSize(i));
    }
}

void TChunkedMegaWad::ResetDocLumps() {
    DocLumpIndexByType_.clear();
    AllDocLumps_.clear();

    TVector<TVector<TWadLumpId>> chunksDocLumps(ChunkMegaWadInfos_.size());
    TVector<TVector<size_t>> chunksDocLumpsMapping(ChunkMegaWadInfos_.size());

    size_t chunkedDocLumpPtr = 0;

    for (size_t i = 0; i < ChunkMegaWadInfos_.size(); ++i) {
        chunksDocLumps[i].reserve(ChunkMegaWadInfos_[i].Info().DocLumps.size());
        for (TStringBuf name : ChunkMegaWadInfos_[i].Info().DocLumps) {
            TWadLumpId value;
            if (TryFromString(name, value)) {
                chunksDocLumps[i].push_back(value);
            }
        }

        chunksDocLumpsMapping[i].resize(chunksDocLumps[i].size());
        ChunkMegaWadInfos_[i].MapDocLumps(chunksDocLumps[i], chunksDocLumpsMapping[i]);

        const size_t prevChunkedDocLumpPtr = chunkedDocLumpPtr;

        for (size_t j = 0; j < chunksDocLumps[i].size(); ++j) {
            if (DocLumpIndexByType_.contains(chunksDocLumps[i][j])) {
                const size_t index = DocLumpIndexByType_[chunksDocLumps[i][j]];
                Y_ENSURE(index < prevChunkedDocLumpPtr);
            } else {
                DocLumpIndexByType_[chunksDocLumps[i][j]] = chunkedDocLumpPtr++;
                AllDocLumps_.push_back(chunksDocLumps[i][j]);
            }
        }
    }

    Sort(AllDocLumps_);

    Y_ENSURE(AllDocLumps_.size() <= MaxDocLumps);

    NeedRemapChunkDocLumps_.resize(ChunkMegaWadInfos_.size());
    Fill(NeedRemapChunkDocLumps_.begin(), NeedRemapChunkDocLumps_.end(), false);
    ChunkDocLumpsRemapping_.resize(ChunkMegaWadInfos_.size());
    for (size_t i = 0; i < ChunkMegaWadInfos_.size(); ++i) {
        TVector<size_t>& remapping = ChunkDocLumpsRemapping_[i];
        remapping.resize(chunkedDocLumpPtr);
        Fill(remapping.begin(), remapping.end(), Max<size_t>());
        for (size_t j = 0; j < chunksDocLumps[i].size(); ++j) {
            remapping[DocLumpIndexByType_.at(chunksDocLumps[i][j])] = chunksDocLumpsMapping[i][j];
        }
        for (size_t j = 0; j < chunkedDocLumpPtr; ++j) {
            if (remapping[j] != j) {
                NeedRemapChunkDocLumps_[i] = true;
                break;
            }
        }
    }

    TVector<NDoom::TMegaWadInfo> infos;
    for (size_t i = 0; i < ChunkMegaWadInfos_.size(); ++i) {
        infos.push_back(ChunkMegaWadInfos_[i].Info());
    }
    FetchMapper_.ConstructInPlace(std::move(infos));
}

template <class Region>
TBlob TChunkedMegaWad::LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const {
    Y_ENSURE(mapping.size() == regions.size());
    Y_ENSURE(mapping.size() <= MaxDocLumps);

    if (Y_UNLIKELY(mapping.empty())) {
        return TBlob();
    }

    if (Y_UNLIKELY(docId >= DocChunkMappingSearcher_->Size())) {
        for (size_t i = 0; i < mapping.size(); ++i) {
            regions[i] = Region();
        }
        return TBlob();
    }

    const TDocChunkMapping docMapping = DocChunkMappingSearcher_->Find(docId);
    Y_ENSURE(docMapping.Chunk() < ChunkMegaWadInfos_.size());

    if (docMapping.LocalDocId() >= ChunkMegaWadInfos_[docMapping.Chunk()].Info().DocCount) {
        for (size_t i = 0; i < mapping.size(); ++i) {
            regions[i] = Region();
        }
        return TBlob();
    }

    return LoadFromChunk(
        docMapping.Chunk(),
        mapping,
        regions,
        [&](const TArrayRef<const size_t>& chunkMapping, TArrayRef<Region> chunkRegions) {
            const TMegaWadCommon& chunk = ChunkMegaWadInfos_[docMapping.Chunk()];
            TBlob result = DocLumpChunkedBlobStorage_->Read(docMapping.Chunk(), chunk.Info().FirstDoc + docMapping.LocalDocId());
            chunk.LoadDocLumpRegions(result, chunkMapping, chunkRegions);
            return result;
        });
}

template <class Region>
TVector<TBlob> TChunkedMegaWad::LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const {
    Y_ASSERT(docIds.size() == regions.size());

    if (Y_UNLIKELY(mapping.empty())) {
        return TVector<TBlob>(docIds.size());
    }

    if (Y_UNLIKELY(docIds.empty())) {
        return TVector<TBlob>();
    }

    if (Y_LIKELY(docIds.size() <= MaxDocs)) {
        TVector<TBlob> result;

        size_t size = 0;
        std::array<bool, MaxDocs> isValidDoc;
        std::array<ui32, MaxDocs> validDocChunks;
        std::array<ui32, MaxDocs> validDocLocalIds;

        for (size_t i = 0; i < docIds.size(); ++i) {
            if (docIds[i] < DocChunkMappingSearcher_->Size()) {
                const TDocChunkMapping docMapping = DocChunkMappingSearcher_->Find(docIds[i]);
                Y_ENSURE(docMapping.Chunk() < ChunkMegaWadInfos_.size());
                isValidDoc[i] = (docMapping.LocalDocId() < ChunkMegaWadInfos_[docMapping.Chunk()].Info().DocCount);
                if (isValidDoc[i]) {
                    validDocChunks[size] = docMapping.Chunk();
                    validDocLocalIds[size] = ChunkMegaWadInfos_[docMapping.Chunk()].Info().FirstDoc + docMapping.LocalDocId();
                    ++size;
                }
            } else {
                isValidDoc[i] = false;
            }
        }

        if (Y_LIKELY(size == docIds.size())) {
            result = DocLumpChunkedBlobStorage_->Read(
                TArrayRef<const ui32>(validDocChunks.data(), size),
                TArrayRef<const ui32>(validDocLocalIds.data(), size));

            for (size_t i = 0; i < size; ++i) {
                LoadFromChunk(
                    validDocChunks[i],
                    mapping,
                    regions[i],
                    [&](const TArrayRef<const size_t>& chunkMapping, TArrayRef<Region> chunkRegions) {
                        ChunkMegaWadInfos_[validDocChunks[i]].LoadDocLumpRegions(result[i], chunkMapping, chunkRegions);
                        return nullptr;
                    });
            }
        } else {
            TVector<TBlob> validDocBlobs = DocLumpChunkedBlobStorage_->Read(
                TArrayRef<const ui32>(validDocChunks.data(), size),
                TArrayRef<const ui32>(validDocLocalIds.data(), size));

            result.resize(docIds.size());

            size_t ptr = 0;
            for (size_t i = 0; i < docIds.size(); ++i) {
                if (isValidDoc[i]) {
                    LoadFromChunk(
                        validDocChunks[ptr],
                        mapping,
                        regions[i],
                        [&](const TArrayRef<const size_t>& chunkMapping, TArrayRef<Region> chunkRegions) {
                            ChunkMegaWadInfos_[validDocChunks[ptr]].LoadDocLumpRegions(validDocBlobs[ptr], chunkMapping, chunkRegions);
                            return nullptr;
                        });

                    result[i] = std::move(validDocBlobs[ptr++]);
                } else {
                    for (size_t j = 0; j < regions[i].size(); ++j) {
                        regions[i][j] = Region();
                    }
                    result[i] = TBlob();
                }
            }
        }

        return result;
    }

    TVector<TBlob> result(docIds.size());
    for (size_t i = 0; i < docIds.size(); ++i) {
        result[i] = LoadDocLumps(docIds[i], mapping, regions[i]);
    }

    return result;
}

ui32 TChunkedMegaWad::Size() const {
    return DocChunkMappingSearcher_->Size();
}

TVector<TWadLumpId> TChunkedMegaWad::GlobalLumps() const {
    return Global_->GlobalLumps();
}

bool TChunkedMegaWad::HasGlobalLump(TWadLumpId type) const {
    return Global_->HasGlobalLump(type);
}

TVector<TWadLumpId> TChunkedMegaWad::DocLumps() const {
    return AllDocLumps_;
}

TBlob TChunkedMegaWad::LoadGlobalLump(TWadLumpId type) const {
    return Global_->LoadGlobalLump(type);
}

void TChunkedMegaWad::MapDocLumps(const TArrayRef<const TWadLumpId>& types, TArrayRef<size_t> mapping) const {
    Y_ENSURE(types.size() == mapping.size());
    for (size_t i = 0; i < types.size(); ++i) {
        mapping[i] = DocLumpIndexByType_.at(types[i]);
    }
}

TBlob TChunkedMegaWad::LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const {
    return TChunkedMegaWad::LoadDocLumpsInternal(docId, mapping, regions);
}

TVector<TBlob> TChunkedMegaWad::LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const {
    return TChunkedMegaWad::LoadDocLumpsInternal(docIds, mapping, regions);
}

void TChunkedMegaWad::LoadDocLumps(
    const TArrayRef<const ui32>& docIds,
    const TArrayRef<const size_t>& mapping,
    std::function<void(size_t, TMaybe<TDocLumpData>&&)> callback) const
{
    TVector<ui32> idMapping(Reserve(docIds.size()));
    TVector<ui32> chunks(Reserve(docIds.size()));
    TVector<ui32> ids(Reserve(docIds.size()));
    TVector<TArrayRef<const char>> regions(mapping.size());

    for (size_t i = 0; i < docIds.size(); ++i) {
        auto emptyAnswer = [&] {
            TDocLumpData data;
            data.Regions = regions;
            callback(i, std::move(data));
        };
        if (docIds[i] >= DocChunkMappingSearcher_->Size()) {
            emptyAnswer();
            continue;
        }

        const TDocChunkMapping docMapping = DocChunkMappingSearcher_->Find(docIds[i]);
        Y_ENSURE(docMapping.Chunk() < ChunkMegaWadInfos_.size());
        if (docMapping.LocalDocId() >= ChunkMegaWadInfos_[docMapping.Chunk()].Info().DocCount) {
            emptyAnswer();
        } else {
            idMapping.push_back(i);
            chunks.push_back(docMapping.Chunk());
            ids.push_back(ChunkMegaWadInfos_[docMapping.Chunk()].Info().FirstDoc + docMapping.LocalDocId());
        }
    }
    DocLumpChunkedBlobStorage_->Read(chunks, ids,
        [&](size_t i, TMaybe<TBlob>&& blob) {
            if (!blob.Defined()) {
                callback(idMapping[i], Nothing());
            } else {
                TDocLumpData data;
                data.Regions = regions;
                data.Blob = LoadFromChunk(
                    chunks[i],
                    mapping,
                    TArrayRef<TArrayRef<const char>>(regions),
                    [&](const TArrayRef<const size_t>& chunkMapping, TArrayRef<TArrayRef<const char>> chunkRegions) {
                        const TMegaWadCommon& chunk = ChunkMegaWadInfos_[chunks[i]];
                        chunk.LoadDocLumpRegions(*blob, chunkMapping, chunkRegions);
                        return std::move(*blob);
                    });
                callback(idMapping[i], std::move(data));
            }
        });

}

TVector<TWadLumpId> TChunkedMegaWad::ChunkGlobalLumps(ui32 chunk) const {
    Y_ENSURE(chunk < ChunkMegaWadInfos_.size());
    const TMegaWadCommon& megaWadInfo = ChunkMegaWadInfos_[chunk];

    TVector<TWadLumpId> res(Reserve(megaWadInfo.Info().GlobalLumps.size()));
    for (auto i : megaWadInfo.Info().GlobalLumps) {
        TWadLumpId value;
        if (TryFromString(i.first, value)) {
            res.push_back(value);
        }
    }

    return res;
}

bool TChunkedMegaWad::HasChunkGlobalLump(ui32 chunk, TWadLumpId type) const {
    Y_ENSURE(chunk < ChunkMegaWadInfos_.size());
    return ChunkMegaWadInfos_[chunk].Info().GlobalLumps.contains(ToString(type));
}

TBlob TChunkedMegaWad::LoadChunkGlobalLump(ui32 chunk, TWadLumpId type) const {
    Y_ENSURE(chunk < ChunkMegaWadInfos_.size());
    TString lumpName = ToString(type);
    return GlobalLumpChunkedBlobStorage_->Read(chunk, ChunkMegaWadInfos_[chunk].Info().GlobalLumps.at(lumpName));
}

ui32 TChunkedMegaWad::Chunks() const {
    return ChunkMegaWadInfos_.size();
}

ui32 TChunkedMegaWad::DocChunk(ui32 docId) const {
    Y_ENSURE(docId < DocChunkMappingSearcher_->Size());
    return DocChunkMappingSearcher_->Find(docId).Chunk();
}

TDocChunkMapping TChunkedMegaWad::DocMapping(ui32 docId) const {
    Y_ENSURE(docId < DocChunkMappingSearcher_->Size());
    return DocChunkMappingSearcher_->Find(docId);
}


TUnanswersStats TChunkedMegaWad::Fetch(TConstArrayRef<ui32> docIds, std::function<void(size_t, TChunkedWadDocLumpLoader*)> cb) const {
    TVector<ui32> idMapping(Reserve(docIds.size()));
    TVector<ui32> chunks(Reserve(docIds.size()));
    TVector<ui32> ids(Reserve(docIds.size()));

    for (size_t i = 0; i < docIds.size(); ++i) {
        if (docIds[i] >= DocChunkMappingSearcher_->Size()) {
            TChunkedWadDocLumpLoader loader;
            cb(i, &loader);
            continue;
        }

        const TDocChunkMapping docMapping = DocChunkMappingSearcher_->Find(docIds[i]);
        Y_ENSURE(docMapping.Chunk() < ChunkMegaWadInfos_.size());
        if (docMapping.LocalDocId() >= ChunkMegaWadInfos_[docMapping.Chunk()].Info().DocCount) {
            TChunkedWadDocLumpLoader loader;
            cb(i, &loader);
        } else {
            idMapping.push_back(i);
            chunks.push_back(docMapping.Chunk());
            ids.push_back(ChunkMegaWadInfos_[docMapping.Chunk()].Info().FirstDoc + docMapping.LocalDocId());
        }
    }
    NDoom::TUnanswersStats stats{static_cast<ui32>(docIds.size()), 0};
    DocLumpChunkedBlobStorage_->Read(chunks, ids,
        [&](size_t i, TMaybe<TBlob>&& blob) {
            if (blob) {
                auto loader = FetchMapper_->GetLoader(std::move(*blob), chunks[i]);
                cb(idMapping[i], &loader);
            } else {
                cb(idMapping[i], nullptr);
                ++stats.NumUnanswers;
            }
        });
    return stats;
}

} // namespace NDoom
