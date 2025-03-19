#pragma once

#include <kernel/search_types/search_types.h>

#include <util/generic/vector.h>

namespace NGroupingAttrs {

struct TAttrWeight {
    TCateg AttrId;
    float Weight;

    struct TLess {
        bool operator() (const TAttrWeight& a, const TAttrWeight& b) const {
            return (a.Weight < b.Weight) || (a.Weight == b.Weight && a.AttrId < b.AttrId);
        }
    };

    TAttrWeight() {
    }

    TAttrWeight(TCateg attrId, float weight)
        : AttrId(attrId)
        , Weight(weight)
    {
    }
};

typedef TVector<TAttrWeight> TAttrWeights;

inline void FilterBestAttrWeights(const TAttrWeights& attrs, TAttrWeights& result) {
    static double eps = 1e-6;
    result.clear();
    for (TAttrWeights::const_reverse_iterator it = attrs.rbegin(); (it != attrs.rend()) && it->Weight + eps > attrs.back().Weight; ++it)
        result.push_back(*it);
}

}
