#pragma once

#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/wad_index_type.h>

#include "offroad_inv_wad_iterator.h"

namespace NDoom {


template <EWadIndexType indexType, class Data, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadInvWadSearcher {
    static constexpr bool HasSub = (PrefixVectorizer::TupleSize != 0);

public:
    using THit = Data;
    using TIterator = TOffroadInvWadIterator<Data, Vectorizer, Subtractor, PrefixVectorizer>;
    using TTable = typename TIterator::TReader::TTable;
    using TModel = typename TIterator::TReader::TModel;
    using TPosition = typename TIterator::TReader::TPosition;

    template <typename...Args>
    TOffroadInvWadSearcher(Args&&...args) {
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
            model.Load(Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel)));
            LocalTable_->Reset(model);

            Table_ = LocalTable_.Get();
        }

        HitsBlob_ = Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Hits));

        if constexpr (HasSub) {
            HitsSubBlob_ = Wad_->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitSub));
        }
    }

    bool Seek(const TPosition& start, const TPosition& end, TIterator* iterator) const {
        if (iterator->Tag() != this) {
            if constexpr (HasSub) {
                iterator->Reader_.Reset(HitsSubBlob_, Table_, HitsBlob_);
            } else {
                iterator->Reader_.Reset(Table_, HitsBlob_);
            }
            iterator->SetTag(this);
        }

        if (!iterator->Reader_.Seek(start, THit(), NOffroad::TSeekPointSeek())) {
            return false;
        }

        iterator->Reader_.SetLimits(start, end);
        return true;
    }

private:
    THolder<TTable> LocalTable_;
    const TTable* Table_ = nullptr;

    const IWad* Wad_ = nullptr;
    TBlob HitsSubBlob_;
    TBlob HitsBlob_;
};


} // namespace NOffroad
