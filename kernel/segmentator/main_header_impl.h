#pragma once

#include "main_features_common.h"

namespace NSegm {
namespace NPrivate {

enum EMainHeaderFeatures {
    MHF_COUNT
};

static_assert(MHF_COUNT == 0, "expect MHF_COUNT == 0");

struct THeaderFeatures : public TFeatureArrayVector {
    THeaderFeatures(size_t maxSegments)
        : TFeatureArrayVector(MHF_COUNT, maxSegments)
    {}

    void Calculate(const TDocContext&, const THeaderSpans&, const TSegmentSpans&) override;
};

TMainHeaderSpans FindMainHeaderSpans(const TDocContext& ctx, THeaderSpans& sp, const TSegmentSpans&,
                                     const TMainContentSpans& mains, const TArticleSpans& arts);

TArticleSpans FindArticles(const THeaderSpans&, const TSegmentSpans&);

}
}

