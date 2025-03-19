#include "pseudo_rand.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/digest/numeric.h>

namespace NSnippets {
    size_t PseudoRand(const TString& url, const THitsInfoPtr& hitsInfo) {
        TVector<ui16> hits;
        if (hitsInfo.Get()) {
            hits = hitsInfo->THSents.empty() ? hitsInfo->LinkHits : hitsInfo->THSents;
        }

        size_t hash = ComputeHash(url);
        for (size_t i = 0; i < hits.size(); ++i) {
            hash = CombineHashes(hash, NumericHash(hits[i]));
        }
        return hash;
    }
}
