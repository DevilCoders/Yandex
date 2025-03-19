#include "wad_item_fetcher.h"

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>

#include <util/generic/xrange.h>

namespace NDoom::NItemStorage {

template <typename T>
void SafeSetMapping(TVector<T>& mapping, size_t index, T value, T defaul = T{}) {
    if (index >= mapping.size()) {
        mapping.resize(index + 1, defaul);
    }
    mapping[index] = std::move(value);
}

TWadItemFetcher::TWadItemFetcher(NDoom::NItemStorage::IBlobStorage* storage)
    : Storage_{storage}
{}

TWadItemFetcher::TWadItemFetcher(THolder<NDoom::NItemStorage::IBlobStorage> storage)
    : StorageHolder_{std::move(storage)}
    , Storage_(StorageHolder_.Get())
{}

void TWadItemFetcher::Load(NPrivate::TItemLumpsRequest& req, std::function<void(size_t item, TStatusOr<TItemLumps> lumps)> cb) const {
    return Storage_->Load(req, [this, req, &cb](size_t i, TStatusOr<NDoom::NItemStorage::TItemBlob> item) {
        if (!item) {
            cb(i, item.Status());
            return;
        }
        if (!ValidateItem(*item)) {
            cb(i, TStatus::Err(EStatusCode::DataLoss, "Checksum mismatch"));
            return;
        }

        cb(i, NDoom::NItemStorage::TItemLumps{
            .Item = std::move(*item),
            .Mapping = NDoom::NItemStorage::TItemLumpsMapping{
                .Remapping = Remappings_.at(item->Chunk.ItemType).at(item->Chunk.Id),
                .NumLumps = NumChunkLumps_.at(item->Chunk.ItemType).at(item->Chunk.Id),
            },
        });
    });
}

void TWadItemFetcher::SetConsistentMapping(TConsistentItemLumpsMapping* consistentMapping) {
    // FIXME(sskvor): Add iterator for TItemTypeMap
    for (NDoom::NItemStorage::TItemType type : ItemTypes_) {
        TChunkedWadDocLumpMapper& localMapper = MapperByItemType_.at(type);
        TVector<TStringBuf> names = localMapper.DocLumpsNames();
        TVector<size_t> localMapping(names.size(), 0);
        localMapper.MapDocLumps(names, localMapping);

        for (auto [name, localId] : Zip(names, localMapping)) {
            NDoom::NItemStorage::TLumpId lump{
                .ItemType = type,
                .LumpName = TString{name},
            };
            size_t globalId = 0;
            consistentMapping->RegisterItemLumps({ &lump, 1 }, { &globalId, 1 });

            SafeSetMapping(LocalToConsistentRemapping_[type], localId, globalId, Max<size_t>());
            SafeSetMapping(ConsistentToLocalRemapping_, globalId, localId, Max<size_t>());
        }

        for (ui32 chunk : xrange(NumChunks_.at(type))) {
            NumChunkLumps_[type].emplace_back(localMapper.GetChunkDocLumpsCount(chunk));

            TConstArrayRef<size_t> localRemapping = localMapper.GetChunkRemapping(chunk);
            TVector<size_t>& globalRemapping = Remappings_[type].emplace_back();

            // consistentMapping :: THashMap<TLumpName, size_t>
            // consistentMapping[lump] = localId (from chunked mapper)
            //
            // Invariant #1 (localRemapping):
            // mapper.MapDocLumps({&lumpName, 1}, {&id, 1});
            // ChunkInfos[chunk].DocLumps[localRemapping[id]] == lumpName
            //
            // Invariant #2 (globalRemapping):
            // globalRemapping[consistentMapping[lumpName]] == localRemapping[id]
            //
            // So:
            // ChunkInfos[chunk].DocLumps[globalRemapping[consistentMapping[lumpName]]] == lumpName
            globalRemapping.resize(ConsistentToLocalRemapping_.size(), Max<size_t>());
            for (auto [localId, globalId] : Enumerate(LocalToConsistentRemapping_[type])) {
                if (globalId == Max<size_t>()) {
                    continue;
                }
                globalRemapping[globalId] = localRemapping[localId];
            }
        }
    }
}

void TWadItemFetcher::AddItemType(TItemType itemType, const TVector<TMegaWadInfo>& chunkInfos) {
    ItemTypes_.push_back(itemType);
    MapperByItemType_[itemType].Reset(chunkInfos);
    BlobCheckerByItemType_[itemType].Reset(&MapperByItemType_.at(itemType));
    NumChunks_[itemType] = chunkInfos.size();
}

bool TWadItemFetcher::ValidateItem(const NDoom::NItemStorage::TItemBlob& item) const {
    if (!item.Blob) {
        // Blob is not needed
        return true;
    }

    const TChunkedWadDocLumpMapper& mapper = MapperByItemType_.at(item.Chunk.ItemType);
    const TCheckingMapper<TChunkedWadDocLumpMapper>& blobChecker = BlobCheckerByItemType_.at(item.Chunk.ItemType);

    return blobChecker.Validate(*item.Blob, mapper.GetLoader(*item.Blob, item.Chunk.Id));
}

} // namespace NDoom::NItemStorage
