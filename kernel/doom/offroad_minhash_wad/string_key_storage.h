#pragma once

#include <kernel/doom/wad/wad_index_type.h>
#include <kernel/doom/wad/wad_writer.h>
#include <kernel/doom/wad/wad.h>

#include <kernel/doom/offroad_key_wad/offroad_key_wad_writer.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_searcher.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_reader.h>

namespace NDoom {

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, typename DataVectorizer, typename DataSubtractor, typename DataSerializer>
class TKeyWadKeyValueWriter {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TKeyWadWriter = TOffroadKeyWadWriter<indexType, Data, DataVectorizer, DataSubtractor, DataSerializer>;
    using TKeyModel = typename TKeyWadWriter::TModel;

public:
    TKeyWadKeyValueWriter(TKeyModel keyModel, IWadWriter* wadWriter)
        : Buffer_{KeySerializer::MaxSize}
        , Writer_{std::move(keyModel), wadWriter}
    {
    }

    void Write(const Key& key, const Data& data) {
        Buffer_.Resize(8);
        size_t len = KeySerializer::Serialize(key, Buffer_);
        Writer_.WriteKey(TStringBuf{Buffer_.data(), len}, data);
    }

    void Finish() {
        Writer_.Finish();
    }

private:
    TBuffer Buffer_;
    TKeyWadWriter Writer_;
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, typename DataVectorizer, typename DataSubtractor, typename DataSerializer, typename DataCombiner>
struct TKeyWadKeyValueSearcher {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TKeyWadSearcher = TOffroadKeyWadSearcher<indexType, Data, DataVectorizer, DataSubtractor, DataSerializer, DataCombiner>;
    using TKeyWadIterator = typename TKeyWadSearcher::TIterator;

public:
    TKeyWadKeyValueSearcher(const IWad* wad)
        : KeySearcher_{wad}
    {
    }

    void Reset(const IWad* wad) {
        KeySearcher_.Reset(wad);
    }

    TMaybe<Data> Find(ui32 index, const Key&, TStringBuf serialized) const {
        TKeyWadIterator iterator;
        if (!KeySearcher_.Seek(index, &iterator)) {
            return Nothing();
        }

        TStringBuf keyRef;
        Data data;
        if (!iterator.ReadKey(&keyRef, &data)) {
            return Nothing();
        }

        if (keyRef != serialized) {
            return Nothing();
        }
        return data;
    }

private:
    TKeyWadSearcher KeySearcher_;
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, typename DataVectorizer, typename DataSubtractor, typename DataSerializer, typename DataCombiner>
class TKeyWadKeyValueReader {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TKeyWadReader = TOffroadKeyWadReader<indexType, Data, DataVectorizer, DataSubtractor, DataCombiner>;

public:
    TKeyWadKeyValueReader(const IWad* wad) {
        Reset(wad);
    }

    void Reset(const IWad* wad) {
        KeyReader_.Reset(wad);
    }

    bool Read(Key* key, Data* data) {
        TStringBuf keyRef;
        if (!KeyReader_.ReadKey(&keyRef, data)) {
            return false;
        }

        KeySerializer::Deserialize(std::as_const(keyRef), key);
        return true;
    }

private:
    TKeyWadReader KeyReader_;
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeySerializer, typename DataVectorizer, typename DataSubtractor, typename DataSerializer, typename DataCombiner>
struct TStringKeyValueStorage {
    using TWriter = TKeyWadKeyValueWriter<indexType, Key, Data, KeySerializer, DataVectorizer, DataSubtractor, DataSerializer>;
    using TReader = TKeyWadKeyValueReader<indexType, Key, Data, KeySerializer, DataVectorizer, DataSubtractor, DataSerializer, DataCombiner>;
    using TSearcher = TKeyWadKeyValueSearcher<indexType, Key, Data, KeySerializer, DataVectorizer, DataSubtractor, DataSerializer, DataCombiner>;
};

} // namespace NDoom
