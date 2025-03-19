#pragma once

#include "tuple_vectorizer.h"

#include <kernel/doom/wad/wad_index_type.h>
#include <kernel/doom/wad/wad_writer.h>
#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/flat/flat_writer.h>

namespace NDoom {

template <EWadIndexType indexType, typename Key, typename Data, typename KeyVectorizer, typename DataVectorizer>
struct TFlatKeyValueWriter {
    using TWriter = NOffroad::TFlatWriter<Data, Key, DataVectorizer, KeyVectorizer>;

public:
    inline static constexpr EWadIndexType IndexType = indexType;

    // For compatibility
    struct TKeyModel {};

public:
    TFlatKeyValueWriter(TKeyModel, IWadWriter* wadWriter)
        : Writer_{wadWriter->StartGlobalLump(TWadLumpId{IndexType, EWadLumpRole::Keys})}
    {
    }

    void Write(const Key& key, const Data& data) {
        Writer_.Write(data, key);
    }

    void Finish() {
        Writer_.Finish();
    }

private:
    TWriter Writer_;
    IWadWriter* WadWriter_ = nullptr;
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeyVectorizer, typename DataVectorizer>
struct TFlatKeyValueSearcher {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TSearcher = NOffroad::TFlatSearcher<Data, Key, DataVectorizer, KeyVectorizer>;

public:
    TFlatKeyValueSearcher(const IWad* wad) {
        Reset(wad);
    }

    void Reset(const IWad* wad) {
        KeySearcher_.Reset(wad->LoadGlobalLump(TWadLumpId{indexType, EWadLumpRole::Keys}));
    }

    TMaybe<Data> Find(ui32 index, const Key& key, TStringBuf) const {
        if (Y_UNLIKELY(index >= KeySearcher_.Size())) {
            return Nothing();
        }
        if (KeySearcher_.ReadData(index) != key) {
            return Nothing();
        }
        return KeySearcher_.ReadKey(index);
    }

private:
    TSearcher KeySearcher_;
};

template <EWadIndexType indexType, typename Key, typename Data, typename KeyVectorizer, typename DataVectorizer>
struct TFlatKeyValueReader {
public:
    inline static constexpr EWadIndexType IndexType = indexType;

    using TSearcher = NOffroad::TFlatSearcher<Data, Key, DataVectorizer, KeyVectorizer>;

public:
    TFlatKeyValueReader(const IWad* wad) {
        Reset(wad);
    }

    void Reset(const IWad* wad) {
        KeySearcher_.Reset(wad->LoadGlobalLump(TWadLumpId{indexType, EWadLumpRole::Keys}));
    }

    bool Read(Key* key, Data* data) {
        if (Pos_ >= KeySearcher_.Size()) {
            return false;
        }

        *data = KeySearcher_.ReadKey(Pos_);
        *key = KeySearcher_.ReadData(Pos_);

        ++Pos_;
        return true;
    }

    TMaybe<Data> Find(ui32 index, const Key& key, TStringBuf) const {
        if (KeySearcher_.ReadKey(index) != key) {
            return Nothing();
        }
        return KeySearcher_.ReadData(index);
    }

private:
    TSearcher KeySearcher_;
    size_t Pos_ = 0;
};

template <EWadIndexType IndexType, typename Key, typename Data, typename KeyVectorizer, typename DataVectorizer>
struct TFlatKeyValueStorage {
    using TWriter = TFlatKeyValueWriter<IndexType, Key, Data, KeyVectorizer, DataVectorizer>;
    using TReader = TFlatKeyValueReader<IndexType, Key, Data, KeyVectorizer, DataVectorizer>;
    using TSearcher = TFlatKeyValueSearcher<IndexType, Key, Data, KeyVectorizer, DataVectorizer>;
};

} // namespace NDoom
