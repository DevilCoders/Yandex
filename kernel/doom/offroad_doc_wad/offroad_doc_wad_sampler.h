#pragma once

#include <library/cpp/offroad/tuple/tuple_sampler.h>


namespace NDoom {


template<class Hit, class Vectorizer, class Subtractor>
class TOffroadDocWadSampler {
    using TSampler = NOffroad::TTupleSampler<Hit, Vectorizer, Subtractor>;

public:
    using THit = Hit;
    using TTable = typename TSampler::TTable;
    using TModel = typename TSampler::TModel;

    enum {
        Stages = TSampler::Stages,
    };

    TOffroadDocWadSampler() {}

    void Reset() {
        Sampler_.Reset();
    }

    void WriteHit(const THit& hit) {
        Sampler_.WriteHit(hit);
    }

    void WriteDoc(ui32) {
        Sampler_.FinishBlock();
    }

    TModel Finish() {
        return Sampler_.Finish();
    }

    bool IsFinished() const {
        return Sampler_.IsFinished();
    }

private:
    TSampler Sampler_;
};


} // namespace NDoom
