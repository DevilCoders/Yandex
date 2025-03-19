#include "wad_blob_storage.h"
#include "wad_item_fetcher.h"
#include "wad_item_lumps_storage.h"
#include "wad_item_storage.h"

#include "blob_storage.h"
#include "item_chunk_mapper.h"
#include "item_fetcher.h"

#include <kernel/doom/blob_storage/mapped_wad_chunked_blob_storage.h>
#include <kernel/doom/blob_storage/direct_aio_wad_chunked_blob_storage.h>
#include <kernel/doom/chunked_wad/mapper.h>
#include <kernel/doom/wad/mega_wad.h>
#include <kernel/doom/wad/wad_signature.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/iterator/enumerate.h>

#include <util/string/builder.h>
#include <util/system/fs.h>

namespace NDoom::NItemStorage {

namespace {

class TWadGlobalLumpsStorage : public IGlobalLumpsStorage {
public:
    TWadGlobalLumpsStorage(THolder<IWad> wad);
    TWadGlobalLumpsStorage(IWad* wad);
    void Init();

    TVector<TLumpNameRef> GlobalLumps() const override;
    bool HasGlobalLump(TLumpNameRef lumpName) const override;
    TBlob LoadGlobalLump(TLumpNameRef lumpName) const override;

private:
    THolder<IWad> LocalWad_;
    IWad* Wad_ = nullptr;

    TVector<TLumpName> GlobalLumps_;
};

TWadGlobalLumpsStorage::TWadGlobalLumpsStorage(THolder<IWad> wad)
    : LocalWad_{ std::move(wad) }
    , Wad_{ LocalWad_.Get() }
{
    Init();
}

TWadGlobalLumpsStorage::TWadGlobalLumpsStorage(IWad* wad)
    : Wad_{ wad }
{
    Init();
}

void TWadGlobalLumpsStorage::Init() {
    GlobalLumps_ = Wad_->GlobalLumpsNames();
}

TVector<TLumpNameRef> TWadGlobalLumpsStorage::GlobalLumps() const {
    return TVector<TLumpNameRef>(GlobalLumps_.begin(), GlobalLumps_.end());
}

bool TWadGlobalLumpsStorage::HasGlobalLump(TLumpNameRef lumpName) const {
    return Wad_->HasGlobalLump(lumpName);
}

TBlob TWadGlobalLumpsStorage::LoadGlobalLump(TLumpNameRef lumpName) const {
    return Wad_->LoadGlobalLump(lumpName);
}

class TUnimplementedGlobalLumpsStorage : public IGlobalLumpsStorage {
public:
    TVector<TLumpNameRef> GlobalLumps() const override {
        // LCOV_EXCL_START
        Y_ENSURE(false, "GlobalLumps() is unimplemented");
        // LCOV_EXCL_STOP
    }

    bool HasGlobalLump(TLumpNameRef /* lumpName */) const override {
        // LCOV_EXCL_START
        Y_ENSURE(false, "HasGlobalLump() is unimplemented");
        // LCOV_EXCL_STOP
    }

    TBlob LoadGlobalLump(TLumpNameRef /* lumpName */) const override {
        // LCOV_EXCL_START
        Y_ENSURE(false, "LoadGlobalLump() is unimplemented");
        // LCOV_EXCL_STOP
    }
};

struct TWadItemStorageCommons {
    TWadItemStorageCommons(const IChunkedBlobStorage* blobStorage)
        : ChunkGlobalStorage{ blobStorage }
    {
        WadCommons.resize(ChunkGlobalStorage->Chunks());
        for (auto&& [i, common] : Enumerate(WadCommons)) {
            common.Reset(ChunkGlobalStorage, i);
        }
    }

    ui32 NumChunks() const {
        return ChunkGlobalStorage->Chunks();
    }

    TVector<TMegaWadInfo> WadInfos() const {
        TVector<TMegaWadInfo> infos(Reserve(WadCommons.size()));
        for (const TMegaWadCommon& common : WadCommons) {
            infos.push_back(common.Info());
        }
        return infos;
    }

public:
    const IChunkedBlobStorage* ChunkGlobalStorage = nullptr;
    TVector<TMegaWadCommon> WadCommons;
};

class TWadChunkGlobalLumpsStorage : public IGlobalLumpsStorage {
public:
    TWadChunkGlobalLumpsStorage(const TWadItemStorageCommons* commons, ui32 chunk);

    TVector<TLumpNameRef> GlobalLumps() const override;
    bool HasGlobalLump(TLumpNameRef lumpName) const override;
    TBlob LoadGlobalLump(TLumpNameRef lumpName) const override;

private:
    const TWadItemStorageCommons* Commons_ = nullptr;
    ui32 Chunk_ = 0;
};

TWadChunkGlobalLumpsStorage::TWadChunkGlobalLumpsStorage(const TWadItemStorageCommons* commons, ui32 chunk)
    : Commons_{ std::move(commons) }
    , Chunk_{ chunk }
{
}

TVector<TLumpNameRef> TWadChunkGlobalLumpsStorage::GlobalLumps() const {
    auto& globalLumps = Commons_->WadCommons[Chunk_].Info().GlobalLumps;
    TVector<TLumpNameRef> lumps(Reserve(globalLumps.size()));
    for (auto [lump, index] : globalLumps) {
        lumps.push_back(lump);
    }
    return lumps;
}

bool TWadChunkGlobalLumpsStorage::HasGlobalLump(TLumpNameRef name) const {
    return Commons_->WadCommons[Chunk_].Info().GlobalLumps.contains(name);
}

TBlob TWadChunkGlobalLumpsStorage::LoadGlobalLump(TLumpNameRef name) const {
    const ui32* index = Commons_->WadCommons[Chunk_].Info().GlobalLumps.FindPtr(name);
    Y_ENSURE(index, "Unknown chunk global lump " << name << " in chunk " << Chunk_);
    return Commons_->ChunkGlobalStorage->Read(Chunk_, *index);
}

////////////////////////////////////////////////////////////////////////////////

class TChunkedWadItemFetcher : public TWadItemFetcher {
public:
    TChunkedWadItemFetcher(THolder<IBlobStorage> loader, TItemType itemType, THolder<TWadItemStorageCommons> commons)
        : TWadItemFetcher{std::move(loader)}
        , Commons_{std::move(commons)}
    {
        AddItemType(itemType, Commons_->WadInfos());
    }

private:
    THolder<TWadItemStorageCommons> Commons_;
};

THolder<IChunkedBlobStorage> MakeBlobStorage(TVector<TString> paths, bool lockMemory, EWadOpenMode mode) {
    switch (mode) {
    case EWadOpenMode::Mapped:
        return MakeHolder<TMappedWadChunkedBlobStorage>(std::move(paths), lockMemory, ToString(MegaWadSignature));
    case EWadOpenMode::DirectAio:
        return MakeHolder<TDirectAioWadChunkedBlobStorage>(std::move(paths), lockMemory, ToString(MegaWadSignature));
    }
}

} // namespace 

TItemStorageBackend MakeWadItemStorageBackend(TItemType itemType, const TFsPath& path, bool lockMemory, EWadOpenMode mode) {
    THolder<IChunkedBlobStorage> blobStorage = MakeBlobStorage(TVector<TString>{ path }, lockMemory, mode);
    THolder<IWad> wad = MakeHolder<TMegaWad>(blobStorage.Get());

    THolder<TWadItemStorageCommons> commons = MakeHolder<TWadItemStorageCommons>(blobStorage.Get());
    Y_ENSURE(commons->NumChunks() == 1, "Only one chunk supported");

    THolder<IBlobStorage> storage = MakeHolder<TChunkedWadItemBlobStorage>(
        TChunkedWadItemBlobStorageBuilder{}
            .SetMapper(MakeHolder<TItemChunkMapper>(wad.Get()))
            .SetWadCommons(commons->WadCommons)
            .SetBlobStorage(std::move(blobStorage)));

    THolder<IItemFetcher> fetcher = MakeHolder<TChunkedWadItemFetcher>(std::move(storage), itemType, std::move(commons));

    return TItemStorageBackendBuilder{}
        .SetGlobalLumpsStorage(MakeIntrusive<TWadGlobalLumpsStorage>(std::move(wad)))
        .SetChunkGlobalLumps({MakeIntrusive<TUnimplementedGlobalLumpsStorage>()})
        .SetItemLumps(MakeIntrusive<TWadItemLumpsStorage>(std::move(fetcher)))
        .SetItemType(itemType)
        .Build();
}

TItemStorageBackend MakeChunkedWadItemStorageBackend(TItemType itemType, const TFsPath& prefix, bool lockMemory, EWadOpenMode mode) {
    THolder<IWad> globalWad = IWad::Open(prefix.GetPath() + ".global.wad", lockMemory);
    THolder<IWad> mappingWad = IWad::Open(prefix.GetPath() + ".mapping.wad", lockMemory);

    TVector<TString> chunkPaths;
    for (ui32 i = 0; ; ++i) {
        TString path = TStringBuilder{} << prefix.GetPath() << "." << i << ".wad";
        if (!NFs::Exists(path)) {
            break;
        }

        chunkPaths.push_back(std::move(path));
    }

    THolder<IChunkedBlobStorage> blobStorage = MakeBlobStorage(chunkPaths, lockMemory, mode);
    THolder<TWadItemStorageCommons> commons = MakeHolder<TWadItemStorageCommons>(blobStorage.Get());
    TVector<IGlobalLumpsStoragePtr> chunkGlobalLumps(Reserve(commons->NumChunks()));
    for (ui32 chunk : xrange(commons->NumChunks())) {
        chunkGlobalLumps.push_back(MakeIntrusive<TWadChunkGlobalLumpsStorage>(commons.Get(), chunk));
    }

    THolder<IBlobStorage> storage = MakeHolder<TChunkedWadItemBlobStorage>(
        TChunkedWadItemBlobStorageBuilder{}
            .SetMapper(MakeHolder<TItemChunkMapper>(std::move(mappingWad)))
            .SetWadCommons(commons->WadCommons)
            .SetBlobStorage(std::move(blobStorage)));

    THolder<IItemFetcher> fetcher = MakeHolder<TChunkedWadItemFetcher>(std::move(storage), itemType, std::move(commons));

    return TItemStorageBackendBuilder{}
        .SetGlobalLumpsStorage(MakeIntrusive<TWadGlobalLumpsStorage>(std::move(globalWad)))
        .SetChunkGlobalLumps(std::move(chunkGlobalLumps))
        .SetItemLumps(MakeIntrusive<TWadItemLumpsStorage>(std::move(fetcher)))
        .SetItemType(itemType)
        .Build();
}

} // namespace NDoom::NItemStorage
