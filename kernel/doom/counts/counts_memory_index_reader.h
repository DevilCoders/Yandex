#pragma once

#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <search/panther/indexing/operations/ordered_term_info.h>
#include <search/panther/mappings/term_mapping.h>
#include <search/panther/mappings/counts_mapping.h>
#include <kernel/doom/progress/progress.h>

#include "hit_count_key_data.h"

namespace NDoom {

class TCountsMemoryIndexReader {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = THitCountKeyData;
    using THit = TCountsHit;

    TCountsMemoryIndexReader(const NPanther::TTermMapping* terms, const TVector<NPanther::TOrderedTermInfo>* keys, NPanther::TCountsMapping* interim)
        : Terms_(terms)
        , Keys_(keys)
        , Interim_(interim)
    {
        Y_ASSERT(terms && keys && interim);
    }

    void Restart() {
        KeyIndex_ = 0;
        HitIndex_ = 0;
    }

    bool ReadKey(TKeyRef* key, TKeyData* data = nullptr) {
        if (KeyIndex_ >= Keys_->size())
            return false;

        if (data) {
            data->HitCount = Interim_->HitsCount((*Keys_)[KeyIndex_].Id);
        }
        NewKey = true;
        Terms_->CopyTerm((*Keys_)[KeyIndex_].Bigram, ' ', &Key_);
        *key = Key_;
        KeyIndex_++;

        return true;
    }

    void ReadHits() {
        Hits_ = Interim_->CopySortedHits((*Keys_)[KeyIndex_ -  1].Id);
        HitIndex_ = 0;
    }

    bool ReadHit(THit* hit) {
        if (NewKey) {
            ReadHits();
            NewKey = false;
        }
        if (HitIndex_ >= Hits_.size())
            return false;

        *hit = Hits_[HitIndex_];

        HitIndex_++;
        return true;
    }

    TProgress Progress() const {
        return TProgress(KeyIndex_, Keys_->size());
    }

private:
    const NPanther::TTermMapping* Terms_;
    const TVector<NPanther::TOrderedTermInfo>* Keys_;
    NPanther::TCountsMapping* Interim_;
    NPanther::TCountsMapping::THitContainer Hits_;
    TString Key_;
    ui32 KeyIndex_ = 0;
    ui32 HitIndex_ = 0;
    bool NewKey = true;
};


} // namespace NDoom
