#pragma once

#include <kernel/doom/wad/wad.h>
#include "offroad_struct_diff_wad_iterator.h"
#include "struct_diff_seeker.h"

namespace NDoom {

template <EWadIndexType indexType, class Key, class KeyVectorizer, class KeySubtractor, class KeyPrefixVectorizer, class Data>
class TOffroadStructDiffWadSearcher {
    using TSeeker32 = TStructDiffSeeker<Key, KeyPrefixVectorizer, Data, ui32, NOffroad::TUi32Vectorizer>;
    using TSeeker64 = TStructDiffSeeker<Key, KeyPrefixVectorizer, Data, ui64, NOffroad::TUi64Vectorizer>;

public:
    using TKey = Key;
    using TData = Data;
    using TIterator = TOffroadStructDiffWadIterator<Key, KeyVectorizer, KeySubtractor, Data>;
    using TTable = typename TIterator::TReader::TTable;
    using TModel = typename TIterator::TReader::TModel;

    TOffroadStructDiffWadSearcher() {}

    template <typename...Args>
    TOffroadStructDiffWadSearcher(Args&&...args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const IWad* wad) {
        Reset(nullptr, wad);
    }

    void Reset(const TTable* table, const IWad* wad) {
        Table_ = table;

        if (!Table_) {
            if (!LocalTable_)
                LocalTable_ = MakeHolder<TTable>();

            TModel model;
            model.Load(wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructModel)));
            LocalTable_->Reset(model);
            Table_ = LocalTable_.Get();
        }

        Data_ = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Struct));

        Use64BitSeeker_ = false;
        if (wad->HasGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel))) {
            TBlob seekerSizeBlob = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel));
            Y_ENSURE(seekerSizeBlob.Size() == sizeof(ui32));
            const ui32 seekerOffsetBytes = ReadUnaligned<ui32>(seekerSizeBlob.Data());
            Y_ENSURE(seekerOffsetBytes == sizeof(ui32) || seekerOffsetBytes == sizeof(ui64));
            Use64BitSeeker_ = (seekerOffsetBytes == sizeof(ui64));
        }

        if (Use64BitSeeker_) {
            Seeker64_.Reset(wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSub)));
        } else {
            Seeker32_.Reset(wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSub)));
        }

        TBlob structSizeBlob = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize));
        Y_ENSURE(structSizeBlob.Size() == sizeof(DataSize_));
        DataSize_ = ReadUnaligned<ui32>(structSizeBlob.Data());
    }

    bool LowerBound(const TKey& key, TKey* firstKey, const TData** firstData, TIterator* iterator) const {
        if (iterator->Tag() != this) {
            iterator->Reader_.Reset(DataSize_, Table_, Data_);
            iterator->SetTag(this);
        }

        if (Use64BitSeeker_) {
            return Seeker64_.LowerBound(key, firstKey, firstData, &iterator->Reader_);
        } else {
            return Seeker32_.LowerBound(key, firstKey, firstData, &iterator->Reader_);
        }
    }

private:
    ui32 DataSize_ = 0;
    THolder<TTable> LocalTable_;
    const TTable* Table_ = nullptr;
    TBlob Data_;

    bool Use64BitSeeker_ = false;
    TSeeker32 Seeker32_;
    TSeeker64 Seeker64_;
};


} // namespace NDoom
