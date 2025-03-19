#pragma once

#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/key/key_sampler.h>
#include <library/cpp/offroad/codec/sampler_64.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NDoom {

template <class Hit, class HitVectorizer, class HitSubtractor>
class TKosherPortionSampler {
public:
    using THit = Hit;
    using THitSampler = NOffroad::TTupleSampler<THit, HitVectorizer, HitSubtractor>;
    using THitModel = typename THitSampler::TModel;

    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeySampler = NOffroad::TKeySampler<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
    using TKeyModel = typename TKeySampler::TModel;

    static_assert(THitSampler::Stages == 1, "Expected 1 stage for hit sampler");
    static_assert(TKeySampler::Stages == 1, "Expected 1 stage for key sampler");

    enum {
        Stages = 1,
    };

    void Reset() {
        HitsCount_ = 0;
        HitSampler_.Reset();
        KeySampler_.Reset();
    }

    void WriteHit(const THit& hit) {
        HitSampler_.WriteHit(hit);
        ++HitsCount_;
    }

    void WriteKey(const TKeyRef& key) {
        if (HitsCount_ > 0) {
            HitSampler_.WriteSeekPoint();
            KeySampler_.WriteKey(key, HitsCount_);
            HitsCount_ = 0;
        }
    }

    std::pair<THitModel, TKeyModel> Finish() {
        return { HitSampler_.Finish(), KeySampler_.Finish() };
    }

private:
    ui32 HitsCount_ = 0;
    THitSampler HitSampler_;
    TKeySampler KeySampler_;
};

} // namespace NDoom
