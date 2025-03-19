#include <kernel/doom/blob_storage/mapped_wad_chunked_blob_storage.h>

#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_varint_serializer.h>
#include <library/cpp/offroad/flat/flat_ui32_searcher.h>

#include <util/string/cast.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>

#include "mega_wad.h"
#include "wad_signature.h"

namespace NDoom {

TMegaWad::TMegaWad(const IChunkedBlobStorage* blobStorage) {
    Reset(blobStorage);
}

TMegaWad::TMegaWad(const TString& path, bool lockMemory) {
    LocalBlobStorage_ = THolder<TMappedWadChunkedBlobStorage>(new TMappedWadChunkedBlobStorage({ path }, lockMemory, ToString(MegaWadSignature)));
    Reset(LocalBlobStorage_.Get());
}

TMegaWad::TMegaWad(const TArrayRef<const char>& source)
    : TMegaWad(TBlob::NoCopy(source.data(), source.size())) {
}

TMegaWad::TMegaWad(const TBlob& blob) {
    TVector<TMappedWadChunk> chunks;
    chunks.emplace_back(TMappedWadChunkDataReader(blob), ToString(MegaWadSignature));

    LocalBlobStorage_ = MakeHolder<TMappedWadChunkedBlobStorage>(std::move(chunks));
    Reset(LocalBlobStorage_.Get());
}

TMegaWad::~TMegaWad() {
}

void TMegaWad::Reset(const IChunkedBlobStorage* blobStorage) {
    Y_ENSURE(blobStorage->Chunks() == 1, "MegaWad supports only single chunk");

    BlobStorage_ = blobStorage;
    TMegaWadCommon::Reset(blobStorage, 0);

    Y_ENSURE_EX(Info().DocCount + Info().FirstDoc < blobStorage->ChunkSize(0), yexception() << "Invalid MegaWad format, doc range doesn't match the actual wad structure");
}

ui32 TMegaWad::Size() const {
    return Info().DocCount;
}

TVector<TWadLumpId> TMegaWad::GlobalLumps() const {
    TVector<TWadLumpId> res;

    res.reserve(Info().GlobalLumps.size());
    for (auto i : Info().GlobalLumps) {
        TWadLumpId value;
        if (TryFromString(i.first, value)) {
            res.push_back(value);
        }
    }

    return res;
}

TVector<TString> TMegaWad::GlobalLumpsNames() const {
    TVector<TString> res;

    res.reserve(Info().GlobalLumps.size());
    for (auto i : Info().GlobalLumps) {
        res.push_back(i.first);
    }

    return res;
}

bool TMegaWad::HasGlobalLump(TWadLumpId type) const {
    return Info().GlobalLumps.contains(ToString(type));
}

bool TMegaWad::HasGlobalLump(TStringBuf type) const {
    return Info().GlobalLumps.contains(type);
}

TVector<TWadLumpId> TMegaWad::DocLumps() const {
    TVector<TWadLumpId> lumps;
    lumps.reserve(Info().DocLumps.size());
    for (TStringBuf lump : Info().DocLumps) {
        TWadLumpId value;
        if (TryFromString(lump, value)) {
            lumps.push_back(value);
        }
    }
    return lumps;
}

TVector<TStringBuf> TMegaWad::DocLumpsNames() const {
    TVector<TStringBuf> lumps;
    lumps.reserve(Info().DocLumps.size());
    for (TStringBuf lump : Info().DocLumps) {
        lumps.push_back(lump);
    }
    return lumps;
}

void TMegaWad::MapDocLumps(const TArrayRef<const TWadLumpId>& types, TArrayRef<size_t> mapping) const {
    TMegaWadCommon::MapDocLumps(types, mapping);
}

void TMegaWad::MapDocLumps(TConstArrayRef<TStringBuf> names, TArrayRef<size_t> mapping) const {
    TMegaWadCommon::MapDocLumps(names, mapping);
}

void TMegaWad::MapDocLumps(TConstArrayRef<TString> names, TArrayRef<size_t> mapping) const {
    TMegaWadCommon::MapDocLumps(names, mapping);
}

TBlob TMegaWad::LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const {
    return TMegaWad::LoadDocLumpsInternal(docId, mapping, regions);
}

TVector<TBlob> TMegaWad::LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const {
    return TMegaWad::LoadDocLumpsInternal(docIds, mapping, regions);
}

TBlob TMegaWad::LoadGlobalLump(TWadLumpId type) const {
    TString lumpName = ToString(type);
    return LoadGlobalLump(lumpName);
}

TBlob TMegaWad::LoadGlobalLump(TStringBuf lumpName) const {
    auto pos = Info().GlobalLumps.find(lumpName);

    Y_ENSURE(pos != Info().GlobalLumps.end(), "Error: " << lumpName << " was not found");

    return BlobStorage_->Read(0, pos->second);
}

template <class Region>
TBlob TMegaWad::LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const {
    Y_ASSERT(mapping.size() == regions.size());

    if (Y_UNLIKELY(docId >= Info().DocCount)) {
        for (size_t i = 0; i < regions.size(); i++) {
            regions[i] = Region();
        }
        return TBlob();
    }

    TBlob result = BlobStorage_->Read(0, docId + Info().FirstDoc);

    TMegaWadCommon::LoadDocLumpRegions(result, mapping, regions);

    return result;
}

template <class Region>
TVector<TBlob> TMegaWad::LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const {
    Y_ASSERT(docIds.size() == regions.size());

    TVector<ui32> validIds;
    validIds.reserve(docIds.size());

    TVector<ui32> validChunks;
    validChunks.reserve(docIds.size());

    TVector<TBlob> returnResult(docIds.size());
    TVector<bool> valid(docIds.size());
    for (size_t i = 0; i < docIds.size(); ++i) {
        if (Y_LIKELY(docIds[i] < Info().DocCount)) {
            validIds.push_back(docIds[i] + Info().FirstDoc);
            validChunks.push_back(0);
            valid[i] = true;
        } else {
            for (size_t j = 0; j < regions[i].size(); ++j) {
                regions[i][j] = Region();
            }
        }
    }

    TVector<TBlob> result = BlobStorage_->Read(validChunks, validIds);

    size_t j = 0;
    for (size_t i = 0; i < docIds.size(); ++i) {
        if (valid[i]) {
            returnResult[i].Swap(result[j++]);
            TMegaWadCommon::LoadDocLumpRegions(returnResult[i], mapping, regions[i]);
        }
    }

    return returnResult;
}

} // namespace NDoom
