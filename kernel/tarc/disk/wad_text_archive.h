#pragma once

#include <robot/jupiter/protos/extarc/extarc_keys.pb.h>

#include <kernel/extarc_compression/compressor.h>

#include <kernel/doom/erasure/lrc.h>
#include <kernel/doom/erasure/identity.h>
#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/mapper.h>
#include <kernel/doom/wad/multi_mapper.h>
#include <kernel/doom/wad/mega_wad.h>
#include <kernel/doom/blob_storage/direct_aio_wad_chunked_blob_storage.h>
#include <kernel/doom/wad/wad_signature.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <util/stream/mem.h>
#include <util/stream/zlib.h>

#include <kernel/tarc/disk/wad_text_archive_compressed_blob.h>
#include <kernel/tarc/disk/wad_text_archive_repacked_blob.h>

#include <kernel/tarc/repack/repack.h>

#include <array>

using WadTextArchiveErasureCodec = NDoom::TLrc<12, 4>;
using TWadTextArchiveCacheCodec = NDoom::TIdentityCodec<1>;

enum EDataType : ui8 {
    None = 0,
    DocText = 1,
    ExtInfo = 2,
};

enum class ECompressionType : ui32 {
    None = 0,
    Zlib = 1,
    Repack = 2,

    Count,
};

class TWadTextArchiveManager {
public:
    TWadTextArchiveManager() = default;

    TWadTextArchiveManager(
        const TString& path,
        EDataType dataType,
        bool lockMemory = false,
        NDoom::TWadLumpId lump = NDoom::TWadLumpId(NDoom::EWadIndexType::ArcIndexType, NDoom::EWadLumpRole::Struct),
        const NDoom::IDocLumpMapper* mapper = nullptr,
        const TVector<THolder<NDoom::IWad>>* chunkGlobals = nullptr,
        const TArrayRef<const NDoom::TWadLumpId> optionalLumps = {})
        : Mapper(mapper)
        , OptionalLumps(optionalLumps.begin(), optionalLumps.end())
    {
        Open(path, dataType, lockMemory, lump);
        LoadCodecs(chunkGlobals);
    }

    TWadTextArchiveManager(
        NDoom::IWad* wad,
        EDataType dataType,
        NDoom::TWadLumpId lump = NDoom::TWadLumpId(NDoom::EWadIndexType::ArcIndexType, NDoom::EWadLumpRole::Struct),
        const TArrayRef<const NDoom::TWadLumpId> optionalLumps = {})
        : DataType(dataType)
        , WadLumpId(lump)
        , Wad(wad)
        , OptionalLumps(optionalLumps.begin(), optionalLumps.end())
    {
        Reset();
    }

    TWadTextArchiveManager(
        THolder<NDoom::IWad> wad,
        EDataType dataType,
        NDoom::TWadLumpId lump = NDoom::TWadLumpId(NDoom::EWadIndexType::ArcIndexType, NDoom::EWadLumpRole::Struct),
        const TArrayRef<const NDoom::TWadLumpId> optionalLumps = {})
        : DataType(dataType)
        , WadLumpId(lump)
        , LocalWad(std::move(wad))
        , Wad(LocalWad.Get())
        , OptionalLumps(optionalLumps.begin(), optionalLumps.end())
    {
        Reset();
    }

    TWadTextArchiveManager(
        const NDoom::IWad* wad,
        const NDoom::IDocLumpMapper* mapper,
        EDataType dataType,
        NDoom::TWadLumpId lump = NDoom::TWadLumpId(NDoom::EWadIndexType::ArcIndexType, NDoom::EWadLumpRole::Struct),
        const TArrayRef<const NDoom::TWadLumpId> optionalLumps = {})
        : DataType(dataType)
        , WadLumpId(lump)
        , Wad(wad)
        , Mapper(mapper)
        , OptionalLumps(optionalLumps.begin(), optionalLumps.end())
    {
        Reset();
    }

    void Reset() {
        MapLumps();

        if (Wad->HasGlobalLump({ WadLumpId.Index, NDoom::EWadLumpRole::StructSub })) {
            Y_ENSURE(DataType == ExtInfo);
            auto blob = Wad->LoadGlobalLump({ WadLumpId.Index, NDoom::EWadLumpRole::StructSub });
            CompressKeys.ConstructInPlace();
            NJupiter::TExtArcKeys keys;
            Y_ENSURE(keys.ParseFromString(TString(blob.AsCharPtr(), blob.Size())));
            for (auto key : keys.GetKeys()) {
                CompressKeys->resize(Max<size_t>(CompressKeys->size(), key.GetId() + 1));
                (*CompressKeys)[key.GetId()] = key.GetKey();
            }
        }

        NDoom::TWadLumpId globalLumpId(WadLumpId.Index, NDoom::EWadLumpRole::StructModel);
        if (Wad->HasGlobalLump(globalLumpId)) {
            TBlob globalLump = Wad->LoadGlobalLump(globalLumpId);
            Y_ASSERT(globalLump.Size() >= sizeof(ECompressionType));
            
            const auto [compType, codec] = CodecFromLump(globalLump);

            switch (compType) {
            case ECompressionType::Zlib:
                IsCompressed = true;
                break;
            case ECompressionType::Repack:
                ChunkToCodec = { codec };
                break;
            default:
                break;
            }
        }
    }

    NRepack::TCodec* TryGetRepackCodec(const ui32 chunk) const {
        if (ChunkToCodec.empty()) {
            return nullptr;
        }

        Y_ENSURE(chunk < ChunkToCodec.size());
        return ChunkToCodec[chunk];
    }

    void Open(const TString& indexPath,
        EDataType dataType,
        bool lockMemory = false,
        NDoom::TWadLumpId lump = NDoom::TWadLumpId(NDoom::EWadIndexType::ArcIndexType, NDoom::EWadLumpRole::Struct))
    {
        DataType = dataType;
        WadLumpId = lump;
        BlobStorage = THolder<NDoom::TDirectAioWadChunkedBlobStorage>(new NDoom::TDirectAioWadChunkedBlobStorage({ indexPath }, lockMemory, ToString(NDoom::MegaWadSignature)));
        LocalWad = MakeHolder<NDoom::TMegaWad>(BlobStorage.Get());
        Wad = LocalWad.Get();
        Reset();
    }

    TMaybe<ui32> GetLumpPosition(NDoom::TWadLumpId lumpId) const {
        if (auto ptr = LumpToIndex.FindPtr(lumpId); ptr) {
            return *ptr;
        }
        return Nothing();
    }

    // Returns subblob of regions[lumpPos] without copy, incrementing refcounter of 'docBlob'
    TBlob GetLumpByPosition(const TBlob& docBlob, TConstArrayRef<TConstArrayRef<char>> regions, const ui32 lumpPos) const {
        ptrdiff_t start = regions[lumpPos].data() - docBlob.AsCharPtr();
        ptrdiff_t end = start + regions[lumpPos].size();
        Y_ASSERT(0 <= start && end <= static_cast<ptrdiff_t>(docBlob.Size()));
        const TBlob rawBlob = docBlob.SubBlob(start, end);

        return rawBlob;
    }

    THolder<TCompressedBlob> GetDocLump(
        const ui32 docId,
        const ui32 lumpPos,
        const bool useCompressions) const
    {
        Y_VERIFY(!Mapper);

        TStackVec<TArrayRef<const char>> regions(Mapping.size());
        const TBlob& docBlob = Wad->LoadDocLumps(docId, Mapping, regions);

        auto maybeCodec = TryGetRepackCodec(0);
        TBlob lump = GetLumpByPosition(docBlob, regions, lumpPos);

        if (maybeCodec) {
            return MakeHolder<TWadTextArchiveRepackedBlob>(std::move(lump), *maybeCodec);
        }
        return MakeHolder<TWadTextArchiveCompressedBlob>(
            std::move(lump),
            useCompressions && IsCompressed,
            DataType == ExtInfo,
            useCompressions ? CompressKeys : Nothing());
    }

    void GetDocLump(
        const TArrayRef<ui32>& docIds,
        std::function<void(size_t, THolder<TCompressedBlob>&&)>&& callback,
        const ui32 lumpPos,
        const bool useCompressions) const
    {
        Y_VERIFY(!Mapper);

        Wad->LoadDocLumps(docIds, Mapping, [this, &callback, &lumpPos, &useCompressions](size_t idx, TMaybe<NDoom::IWad::TDocLumpData>&& docLumpData) {
            if (docLumpData.Defined()) {
                auto maybeCodec = TryGetRepackCodec(0);
                TBlob lump = GetLumpByPosition(docLumpData.Get()->Blob, docLumpData.Get()->Regions, lumpPos);
                if (maybeCodec) {
                    callback(idx, MakeHolder<TWadTextArchiveRepackedBlob>(std::move(lump), *maybeCodec));
                } else {
                    callback(
                        idx,
                        MakeHolder<TWadTextArchiveCompressedBlob>(std::move(lump),
                        useCompressions && IsCompressed,
                        DataType == ExtInfo,
                        useCompressions ? CompressKeys : Nothing()));
                }
            } else {
                callback(idx, MakeHolder<TCompressedBlob>());
            }
        });
    }

    THolder<TCompressedBlob> GetDocLump(
        const NDoom::IDocLumpLoader& loader,
        const ui32 lumpPos,
        const bool useCompressions) const
    {
        TStackVec<TArrayRef<const char>> regions(Mapping.size());
        loader.LoadDocRegions(Mapping, regions);
        auto maybeCodec = TryGetRepackCodec(loader.Chunk());
        TBlob lump = NDoom::MakeBlobHolder({loader.DataHolder()}, regions[lumpPos]);

        if (maybeCodec) {
            return MakeHolder<TWadTextArchiveRepackedBlob>(std::move(lump), *maybeCodec);
        }
        return MakeHolder<TWadTextArchiveCompressedBlob>(
            std::move(lump),
            useCompressions && IsCompressed,
            DataType == ExtInfo,
            useCompressions ? CompressKeys : Nothing());
    }

    THolder<TCompressedBlob> GetDocFullInfo(ui32 docId) const {
        return GetDocLump(docId, 0, true);
    }

    void GetDocFullInfo(const TArrayRef<ui32>& docIds, std::function<void(size_t, THolder<TCompressedBlob>&&)>&& callback) const {
        GetDocLump(docIds, std::move(callback), 0, true);
    }

    THolder<TCompressedBlob> GetBertEmbedding(const NDoom::IDocLumpLoader& loader) const {
        static constexpr NDoom::TWadLumpId BertLumpId{ NDoom::EWadIndexType::BertEmbeddingIndexType, NDoom::EWadLumpRole::Struct };
        return GetOptionalLump(loader, EDataType::ExtInfo, BertLumpId);
    }

    THolder<TCompressedBlob> GetBertEmbedding(ui32 docId) const {
        static constexpr NDoom::TWadLumpId BertLumpId{ NDoom::EWadIndexType::BertEmbeddingIndexType, NDoom::EWadLumpRole::Struct };
        return GetOptionalLump(docId, EDataType::ExtInfo, BertLumpId);
    }

    THolder<TCompressedBlob> GetBertEmbeddingV2(const NDoom::IDocLumpLoader& loader) const {
        static constexpr NDoom::TWadLumpId BertV2LumpId{ NDoom::EWadIndexType::BertEmbeddingV2IndexType, NDoom::EWadLumpRole::Struct };
        return GetOptionalLump(loader, EDataType::ExtInfo, BertV2LumpId);
    }

    THolder<TCompressedBlob> GetBertEmbeddingV2(ui32 docId) const {
        static constexpr NDoom::TWadLumpId BertV2LumpId{ NDoom::EWadIndexType::BertEmbeddingV2IndexType, NDoom::EWadLumpRole::Struct };
        return GetOptionalLump(docId, EDataType::ExtInfo, BertV2LumpId);
    }

    THolder<TCompressedBlob> GetExtInfo(const NDoom::IDocLumpLoader& loader) const {
        if (DataType == EDataType::ExtInfo) {
            return GetDocLump(loader, 0, true);
        } else {
            return MakeHolder<TCompressedBlob>();
        }
    }

    THolder<TCompressedBlob> GetExtInfo(ui32 docId) const {
        if (DataType == EDataType::ExtInfo) {
            return GetDocLump(docId, 0, true);
        } else {
            return MakeHolder<TCompressedBlob>();
        }
    }

    THolder<TCompressedBlob> GetDocText(const NDoom::IDocLumpLoader& loader) const {
        if (DataType == EDataType::DocText) {
            return GetDocLump(loader, 0, true);
        } else {
            return MakeHolder<TCompressedBlob>();
        }
    }

    THolder<TCompressedBlob> GetDocText(ui32 docId) const {
        if (DataType == EDataType::DocText) {
            return GetDocLump(docId, 0, true);
        } else {
            return MakeHolder<TCompressedBlob>();
        }
    }

    void GetDocText(const TArrayRef<ui32>& docIds, std::function<void(size_t, THolder<TCompressedBlob>&&)>&& callback) const {
        if (DataType == EDataType::DocText) {
            GetDocLump(docIds, std::move(callback), 0, true);
            return;
        }

        for (size_t i = 0; i < docIds.size(); ++i) {
            callback(i, MakeHolder<TCompressedBlob>());
        }
    }

    bool IsInArchive(ui32 docId) const {
        return docId < Wad->Size();
    }

    ui32 GetDocCount() const {
        return Wad->Size();
    }

    bool IsOpen() const {
        return Wad != nullptr;
    }

    int UnpackDoc(ui32 docId, TBuffer* out) const {
        out->Clear();
        GetAllText(GetDocText(docId)->UncompressBlob().AsUnsignedCharPtr(), out);
        return true;
    }

    int ObtainExtInfo(ui32 docId, TDocArchive &dA) const {
        if (IsInArchive(docId)) {
            dA.Exists = true;
            TBlob localBlob = GetExtInfo(docId)->UncompressBlob();
            dA.Blobsize = localBlob.Size();
            dA.Blob = TBuffer(localBlob.AsCharPtr(), localBlob.Size());
            return 0;
        } else {
            return -1;
        }
    }

private:
    void MapLumps() {
        const NDoom::IDocLumpMapper* mapper = Mapper ? Mapper : Wad;

        TVector<NDoom::TWadLumpId> availableLumps = mapper->DocLumps();
        TStackVec<NDoom::TWadLumpId> lumpsToMap;

        Y_ENSURE(IsIn(availableLumps, WadLumpId), "Required lump is missing");
        lumpsToMap.push_back(WadLumpId); // must always be the first lump in this vector

        for (NDoom::TWadLumpId lump : OptionalLumps) {
            if (IsIn(availableLumps, lump)) {
                lumpsToMap.push_back(lump);
            }
        }

        Mapping.resize(lumpsToMap.size());
        mapper->MapDocLumps(lumpsToMap, Mapping);

        for (ui32 i = 0; i < lumpsToMap.size(); ++i) {
            LumpToIndex[lumpsToMap[i]] = i;
        }
    }

    THolder<TCompressedBlob> GetOptionalLump(ui32 docId, EDataType dataType, NDoom::TWadLumpId lumpId) const {
        if (DataType == dataType) {
            const TMaybe<ui32> position = GetLumpPosition(lumpId);
            if (position.Defined()) {
                return GetDocLump(docId, *position, false);
            }
        }
        return MakeHolder<TCompressedBlob>();
    }

    THolder<TCompressedBlob> GetOptionalLump(const NDoom::IDocLumpLoader& loader, EDataType dataType, NDoom::TWadLumpId lumpId) const {
        if (DataType == dataType) {
            const TMaybe<ui32> position = GetLumpPosition(lumpId);
            if (position) {
                return GetDocLump(loader, position.GetRef(), false);
            }
        }
        return MakeHolder<TCompressedBlob>();
    }

    void LoadCodecs(const TVector<THolder<NDoom::IWad>>* chunkGlobalsPtr) {
        if (!chunkGlobalsPtr || DataType != EDataType::DocText) {
            return;
        }
        const auto& chunkGlobals = *chunkGlobalsPtr;

        ChunkToCodec.clear();
        ChunkToCodec.resize(chunkGlobals.size());

        NDoom::TWadLumpId codecInfoLumpId(WadLumpId.Index, NDoom::EWadLumpRole::StructModel);
        for (size_t i = 0; i < chunkGlobals.size(); ++i) {
            if (!chunkGlobals[i]->HasGlobalLump(codecInfoLumpId)) {
                continue;
            }

            TBlob lump = chunkGlobals[i]->LoadGlobalLump(codecInfoLumpId);
            const auto [compType, codec] = CodecFromLump(lump);
            ChunkToCodec[i] = codec;
        }
    }

    static std::pair<ECompressionType, NRepack::TCodec*> CodecFromLump(const TBlob& lump) {
            const ECompressionType compType = *reinterpret_cast<const ECompressionType*>(lump.AsCharPtr());
            Y_ASSERT(compType < ECompressionType::Count);

            if (compType != ECompressionType::Repack) {
                return {compType, nullptr};
            }

            auto codecInfo = NRepack::NFbs::GetTCodecInfo(reinterpret_cast<const ui8*>(lump.AsCharPtr() + sizeof(compType)));
            return {compType, &NRepack::TCodecWithInfo(codecInfo->Version()->str()).codec};
    }

private:
    EDataType DataType = EDataType::None;
    NDoom::TWadLumpId WadLumpId;
    THolder<NDoom::TDirectAioWadChunkedBlobStorage> BlobStorage;
    THolder<NDoom::IWad> LocalWad;
    const NDoom::IWad* Wad = nullptr;
    TStackVec<size_t> Mapping;
    bool IsCompressed = false;
    TMaybe<TVector<TString>> CompressKeys;
    const NDoom::IDocLumpMapper* Mapper = nullptr;
    TStackVec<NDoom::TWadLumpId> OptionalLumps;
    THashMap<NDoom::TWadLumpId, ui32> LumpToIndex;
    TVector<NRepack::TCodec*> ChunkToCodec;
};
