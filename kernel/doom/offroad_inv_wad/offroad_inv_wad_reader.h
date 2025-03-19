#pragma once

#include <kernel/doom/wad/wad_lump_id.h>
#include <kernel/doom/wad/wad.h>

#include "offroad_inv_common.h"

namespace NDoom {


template <EWadIndexType indexType, class Data, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadInvWadReader {
    using TReader = typename TOffroadInvCommon<Data, Vectorizer, Subtractor, PrefixVectorizer>::TReader;
    static constexpr bool HasSub = (PrefixVectorizer::TupleSize != 0);

public:
    using THit = Data;
    using TModel = typename TReader::TModel;
    using TTable = typename TReader::TTable;
    using TPosition = typename TReader::TPosition;

    enum {
        HasLowerBound = HasSub
    };

    TOffroadInvWadReader() {

    }

    TOffroadInvWadReader(const IWad* wad) {
        Reset(wad);
    }

    void Reset(const IWad* wad) {
        if (!LocalTable_) {
            LocalTable_ = MakeHolder<TTable>();
        }

        TModel model;
        model.Load(wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel)));
        LocalTable_->Reset(model);

        ResetInternal(wad, std::integral_constant<bool, HasSub>());
    }

    bool Seek(const TPosition& start, const TPosition& end) {
        if (!Reader_.Seek(start, THit(), NOffroad::TSeekPointSeek())) {
            return false;
        }
        Reader_.SetLimits(start, end);
        return true;
    }

    bool ReadHit(THit* data) {
        return Reader_.ReadHit(data);
    }

    bool LowerBound(const THit prefix, const THit* first) {
        return Reader_.LowerBound(prefix, first);
    }

private:
    void ResetInternal(const IWad* wad, std::false_type) {
        HitsBlob_ = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Hits));
        Reader_.Reset(LocalTable_.Get(), HitsBlob_);
    }

    void ResetInternal(const IWad* wad, std::true_type) {
        SubBlob_ = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitSub));
        HitsBlob_ = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Hits));
        Reader_.Reset(SubBlob_, LocalTable_.Get(), HitsBlob_);
    }

private:
    THolder<TTable> LocalTable_;
    TBlob SubBlob_;
    TBlob HitsBlob_;
    TReader Reader_;
};


} // namespace NDoom
