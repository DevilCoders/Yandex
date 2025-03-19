#include "metasearch_fixlist_labels.h"

#include <util/generic/vector.h>
#include <util/string/join.h>


TString GetMSFLBitMaskReadableRepr(const TMetaSearchFixlistLabelsBitMask& mask) {
    TVector<TString> labels;
    for (auto label : mask)
        labels.push_back(ToString(label));
    return JoinSeq("_", labels);
}
