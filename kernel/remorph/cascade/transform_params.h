#pragma once

#include <kernel/remorph/common/gztfilter.h>
#include <kernel/remorph/core/core.h>
#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/containers/sorted_vector/sorted_vector.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCascade {

typedef NSorted::TSortedVector<NGzt::TArticleId> TArticleIdSet;

template <class TSymbolPtr>
struct TTransformParams {
    NRemorph::TInput<TSymbolPtr>& Input;
    const NGztSupport::TGazetteerFilter& DominantArticles;
    TArticleIdSet ProcessedCascades;

    TTransformParams(NRemorph::TInput<TSymbolPtr>& input, const NGztSupport::TGazetteerFilter& dominants)
        : Input(input)
        , DominantArticles(dominants)
    {
    }
};

} // NCascade
