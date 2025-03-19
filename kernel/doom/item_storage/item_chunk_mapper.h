#pragma once

#include "types.h"

#include <kernel/doom/chunked_wad/chunked_wad.h>
#include <kernel/doom/offroad_key_wad/combiners.h>
#include <kernel/doom/offroad_minhash_wad/flat_key_storage.h>
#include <kernel/doom/offroad_minhash_wad/offroad_minhash_wad_io.h>
#include <kernel/doom/offroad_minhash_wad/offroad_minhash_wad_searcher.h>
#include <kernel/doom/offroad_minhash_wad/string_key_storage.h>

#include <library/cpp/offroad/custom/null_serializer.h>
#include <library/cpp/threading/thread_local/thread_local.h>

#include <util/folder/path.h>
#include <util/generic/variant.h>

namespace NDoom::NItemStorage {

struct TItemChunk {
    TChunkId Chunk;
    ui32 Index = 0;

    auto operator<=>(const TItemChunk& rhs) const = default;
};

class IItemChunkMapper {
public:
    virtual ~IItemChunkMapper() = default;

    virtual TMaybe<TItemChunk> GetItemChunk(TItemId item) const = 0;
};

struct TChunkLocalId {
    ui32 Chunk = 0;
    ui32 LocalId = 0;
};

struct TChunkLocalIdVectorizer {
public:
    enum {
        TupleSize = 2
    };

    template <class Slice>
    Y_FORCE_INLINE static void Gather(Slice&& slice, TChunkLocalId* data) {
        data->Chunk = slice[0];
        data->LocalId = slice[1];
    }

    template <class Slice>
    Y_FORCE_INLINE static void Scatter(TChunkLocalId data, Slice&& slice) {
        slice[0] = data.Chunk;
        slice[1] = data.LocalId;
    }
};

using TFlatItemChunkKeyIo = TFlatKeyValueStorage<
    EWadIndexType::ItemStorage,
    TItemKey,
    TChunkLocalId,
    TItemKeyVectorizer,
    TChunkLocalIdVectorizer
>;

using TStringItemChunkKeyIo = TStringKeyValueStorage<
    EWadIndexType::ItemStorage,
    TItemKey,
    TChunkLocalId,
    NOffroad::NMinHash::TTriviallyCopyableKeySerializer<TItemKey>,
    TChunkLocalIdVectorizer,
    NOffroad::TINSubtractor,
    NOffroad::TNullSerializer,
    TIdentityCombiner
>;

using TLegacyItemChunkMappingIo = TOffroadMinHashWadIo<
    EWadIndexType::ItemStorage,
    TItemKey,
    TChunkLocalId,
    NOffroad::NMinHash::TTriviallyCopyableKeySerializer<TItemKey>,
    TStringItemChunkKeyIo
>;

using TItemChunkMappingIo = TOffroadMinHashWadIo<
    EWadIndexType::ItemStorage,
    TItemKey,
    TChunkLocalId,
    NOffroad::NMinHash::TTriviallyCopyableKeySerializer<TItemKey>,
    TFlatItemChunkKeyIo
>;

class TItemChunkMapper : public IItemChunkMapper {
    using TLegacyItemMappingSearcher = TLegacyItemChunkMappingIo::TSearcher;
    using TItemMappingSearcher = TItemChunkMappingIo::TSearcher;

public:
    TItemChunkMapper(IWad* mappingWad) {
        Reset(mappingWad);
    }

    TItemChunkMapper(THolder<IWad> mappingWad)
        : LocalWad_{ std::move(mappingWad) }
    {
        Reset(LocalWad_.Get());
    }

    TItemChunkMapper(const TFsPath& path, bool lockMemory)
        : TItemChunkMapper{ NDoom::IWad::Open(path, lockMemory) }
    {}

    void Reset(const NDoom::IWad* wad) {
        Wad_ = wad;
        if (wad->HasGlobalLump(TWadLumpId{EWadIndexType::ItemStorage, EWadLumpRole::KeyIdx})) {
            Mapping_.emplace<TLegacyItemMappingSearcher>(Wad_);
        } else {
            Mapping_.emplace<TItemMappingSearcher>(Wad_);
        }
    }

    TMaybe<TItemChunk> GetItemChunk(TItemId item) const override {
        auto res = std::visit([&item]<typename T>(const T& mapping) -> TMaybe<TOffroadMinHashWadSearchResult<TChunkLocalId>> {
            if constexpr (std::is_same_v<T, std::monostate>) {
                Y_VERIFY(false);
            } else {
                return mapping.Find(item.ItemKey);
            }
        }, Mapping_);

        if (res) {
            return TItemChunk{
                .Chunk = TChunkId{
                    .ItemType = item.ItemType,
                    .Id = res->Value.Chunk,
                },
                .Index = res->Value.LocalId,
            };
        }
        return Nothing();
    }

private:
    THolder<IWad> LocalWad_;
    const IWad* Wad_ = nullptr;
    std::variant<std::monostate, TLegacyItemMappingSearcher, TItemMappingSearcher> Mapping_;
};

} // namespace NDoom::NItemStorage
