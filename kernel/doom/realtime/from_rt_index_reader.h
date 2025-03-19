#pragma once

#include <array>

#include <util/generic/yexception.h>

#include <kernel/doom/hits/superlong_hit.h>
#include <kernel/doom/progress/progress.h>

#include <yweb/realtime/indexer/common/indexing/index_reader.h>

namespace NDoom {

class TFromRtIndexReader {
public:
    using THit = TSuperlongHit;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = void;

    enum {
        HasLowerBound = false
    };

    TFromRtIndexReader(NRealTime::IRTIndexReader* reader)
        : BaseReader_(reader)
    {
        KeyStorage_[0] = 0;
    }

    bool ReadKey(TKeyRef* key, TKeyData* = NULL) {
        if (FirstKey_) {
            FirstKey_ = false;
        } else {
            BaseReader_->NextKey();
        }

        if (BaseReader_->IsFinished())
            return false;

        BaseReader_->LoadKey(KeyStorage_);

        *key = KeyStorage_;
        return true;
    }

    bool ReadHit(THit* hit) {
        if (HitPos_ == HitCount_) {
            HitPos_ = 0;
            HitCount_ = BaseReader_->LoadHits(Hits_.data(), Hits_.data() + Hits_.size()) - Hits_.data();

            if (HitPos_ == HitCount_)
                return false;
        }

        *hit = Hits_[HitPos_++];
        return true;
    }

    void LowerBound(const TKey&) {
        ythrow yexception() << "LowerBound is not implemented by this reader.";
    }

    TProgress Progress() const {
        return TProgress(BaseReader_->IsFinished() ? 0 : 1, 1);
    }

private:
    bool FirstKey_ = true;
    NRealTime::IRTIndexReader* BaseReader_;
    NRealTime::IRTIndexReader::TKeyStorage KeyStorage_;

    std::array<SUPERLONG, 32> Hits_;
    size_t HitCount_ = 0;
    size_t HitPos_ = 0;
};


} // namespace NDoom
