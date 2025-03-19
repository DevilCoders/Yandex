#pragma once

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/key/fat_key_seeker.h>

#include "offroad_key_wad_iterator.h"

namespace NDoom {


template <EWadIndexType indexType, class KeyData, class Vectorizer, class Subtractor, class Serializer, class Combiner>
class TOffroadKeyWadSearcher {
    using TKeySeeker = NOffroad::TFatKeySeeker<KeyData, Serializer>;

public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = KeyData;
    using TIterator = TOffroadKeyWadIterator<KeyData, Vectorizer, Subtractor, Combiner>;

    using TTable = typename TIterator::TKeyReader::TTable;
    using TModel = typename TIterator::TKeyReader::TModel;

    template <typename...Args>
    TOffroadKeyWadSearcher(Args&&...args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const IWad* wad) {
        Reset(nullptr, wad);
    }

    void Reset(const TTable* table, const IWad* wad) {
        Table_ = table;

        Wad_ = wad;

        if (!Table_) {
            if (!LocalTable_) {
                LocalTable_ = MakeHolder<TTable>();
            }

            TModel model;
            model.Load(Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeysModel)));
            LocalTable_->Reset(model);

            Table_ = LocalTable_.Get();
        }

        KeySeeker_.Reset(Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyFat)), Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::KeyIdx)));
        KeysBlob_ = Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Keys));
    }

    bool Seek(size_t keyIndex, TIterator* iterator) const {
        if (iterator->Tag() != this) {
            iterator->KeyReader_.Reset(Table_, KeysBlob_);
            iterator->SetTag(this);
        }

        return KeySeeker_.Seek(keyIndex, &iterator->KeyReader_);
    }

    bool Find(const TKeyRef& key, TKeyData* data, TIterator* iterator) const {
        TKeyRef firstKey;
        if (!LowerBound(key, &firstKey, data, iterator)) {
            return false;
        }
        return (key == firstKey);
    }

    bool LowerBound(const TKeyRef& key, TKeyRef* firstKey, TKeyData* firstData, TIterator* iterator, size_t* index = nullptr) const {
        if (iterator->Tag() != this) {
            iterator->KeyReader_.Reset(Table_, KeysBlob_);
            iterator->SetTag(this);
        }

        if (Combiner::IsIdentity) {
            return KeySeeker_.LowerBound(key, firstKey, firstData, &iterator->KeyReader_, index);
        }

        TKeyData localData;
        if (!KeySeeker_.LowerBound(key, firstKey, &localData, &iterator->KeyReader_, index)) {
            return false;
        }
        Combiner::Combine(iterator->KeyReader_.LastData(), localData, firstData);
        return true;
    }

private:
    THolder<TTable> LocalTable_;
    const TTable* Table_ = nullptr;

    const IWad* Wad_ = nullptr;

    TKeySeeker KeySeeker_;
    TBlob KeysBlob_;
};


} // namespace NOffroad
