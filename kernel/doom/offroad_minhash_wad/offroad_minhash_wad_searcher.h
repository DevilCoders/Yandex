#pragma once

#include <kernel/doom/offroad_key_wad/offroad_key_wad_searcher.h>
#include <kernel/doom/wad/mega_wad_writer.h>

#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/minhash/searcher.h>

#include <util/stream/input.h>

namespace NDoom {

template <typename T, typename Key, typename Data>
concept MinHashKeyValueSearcher = requires (T t, Key& key, Data& data, const IWad* wad, TStringBuf serialized) {
    t.Reset(wad);
    std::as_const(t).Find(0u, key, serialized);
};

template <typename Data>
struct TOffroadMinHashWadSearchResult {
    ui32 Index = 0;
    Data Value = Data{};
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, MinHashKeyValueSearcher<Key, Data> KeySearcher>
class TOffroadMinHashWadSearcher {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TMinHashSearcher = typename NOffroad::NMinHash::TMinHashSearcher<Key, KeySerializer, /* HasCheckSum */ false>;
    using TResult = TOffroadMinHashWadSearchResult<Data>;

public:
    TOffroadMinHashWadSearcher(const IWad* wad)
        : KeySearcher_{ wad }
    {
        Reset(wad);
    }

    void Reset(const IWad* wad) {
        NOffroad::NMinHash::TMinHashStorage storage;
        storage.Buckets = wad->LoadGlobalLump(LumpId(EWadLumpRole::Hits));
        storage.EmptySlots = wad->LoadGlobalLump(LumpId(EWadLumpRole::Struct));
        storage.Header = wad->LoadGlobalLump(LumpId(EWadLumpRole::StructSub));

        Searcher_.Reset(storage);
        KeySearcher_.Reset(wad);
    }

    TMaybe<TOffroadMinHashWadSearchResult<Data>> Find(const Key& key) const {
        TTempBuf buf{ KeySerializer::MaxSize };
        size_t len = KeySerializer::Serialize(key, TArrayRef<char>{ buf.Data(), buf.Size() });
        TStringBuf rawKey{ buf.Data(), len };

        ui32 index = Searcher_.FindRaw(rawKey);
        TMaybe<Data> res = KeySearcher_.Find(index, key, rawKey);
        if (!res) {
            return Nothing();
        }
        return TResult{
            .Index = index,
            .Value = std::move(*res),
        };
    }

private:
    static TWadLumpId LumpId(EWadLumpRole role) {
        return TWadLumpId{ IndexType, role };
    }

private:
    TMinHashSearcher Searcher_;
    KeySearcher KeySearcher_;
};

} // namespace NDoom
