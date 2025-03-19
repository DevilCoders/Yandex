#pragma once

#include <kernel/doom/wad/mega_wad_info.h>

#include "blob_storage.h"
#include "item_fetcher.h"
#include "item_type_map.h"

namespace NDoom::NItemStorage {

class TWadItemFetcher : public IItemFetcher {
public:
    TWadItemFetcher(IBlobStorage* storage);
    TWadItemFetcher(THolder<IBlobStorage> storage);

    void Load(NPrivate::TItemLumpsRequest& req, std::function<void(size_t item, TStatusOr<TItemLumps> lumps)> cb) const override;

    void SetConsistentMapping(TConsistentItemLumpsMapping* mapping) override;

protected:
    void AddItemType(TItemType itemType, const TVector<TMegaWadInfo>& chunkInfos);

    bool ValidateItem(const NDoom::NItemStorage::TItemBlob& item) const;

protected:
    THolder<IBlobStorage> StorageHolder_;
    IBlobStorage* Storage_;
    TSmallVec<TItemType> ItemTypes_;
    TItemTypeMap<TChunkedWadDocLumpMapper> MapperByItemType_;
    TItemTypeMap<TCheckingMapper<TChunkedWadDocLumpMapper>> BlobCheckerByItemType_;
    TItemTypeMap<size_t> NumChunks_;

    TItemTypeMap<TVector<size_t>> LocalToConsistentRemapping_;
    TVector<size_t> ConsistentToLocalRemapping_;

    // FIXME(sskvor): SoA -> AoS
    TItemTypeMap<TVector<TVector<size_t>>> Remappings_;
    TItemTypeMap<TVector<size_t>> NumChunkLumps_;
};

} // namespace NDoom::NItemStorage
