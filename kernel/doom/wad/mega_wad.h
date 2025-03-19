#pragma once

#include <kernel/doom/blob_storage/chunked_blob_storage.h>

#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/memory/blob.h>

#include "wad.h"
#include "mega_wad_common.h"

namespace NDoom {


/**
 * Implementation of `IWad` interface for megawads, that basically can contain
 * just about anything.
 */
class TMegaWad
    : public TMegaWadCommon
    , public IWad
{
public:
    TMegaWad(const IChunkedBlobStorage* blobStorage);
    TMegaWad(const TString& path, bool lockMemory);
    TMegaWad(const TArrayRef<const char>& source);
    TMegaWad(const TBlob& blob);
    virtual ~TMegaWad() override;

    ui32 Size() const override;
    TVector<TWadLumpId> GlobalLumps() const override;
    TVector<TString> GlobalLumpsNames() const override;
    bool HasGlobalLump(TWadLumpId type) const override;
    bool HasGlobalLump(TStringBuf type) const override;
    TVector<TWadLumpId> DocLumps() const override;
    TVector<TStringBuf> DocLumpsNames() const override;
    void MapDocLumps(const TArrayRef<const TWadLumpId>& types, TArrayRef<size_t> mapping) const override;
    void MapDocLumps(TConstArrayRef<TStringBuf> names, TArrayRef<size_t> mapping) const override;
    void MapDocLumps(TConstArrayRef<TString> names, TArrayRef<size_t> mapping) const override;
    TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const override;
    TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const override;
    TBlob LoadGlobalLump(TWadLumpId type) const override;
    TBlob LoadGlobalLump(TStringBuf type) const override;
private:
    void Reset(const IChunkedBlobStorage* blobStorage);

    template <class Region>
    TBlob LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const;
    template <class Region>
    TVector<TBlob> LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const;


    THolder<IChunkedBlobStorage> LocalBlobStorage_;
    const IChunkedBlobStorage* BlobStorage_ = nullptr;
};


} // namespace NDoom
