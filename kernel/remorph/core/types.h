#pragma once

#include <util/generic/ylimits.h>
#include <util/stream/output.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NRemorph {

typedef ui16 TTagId;
typedef ui32 TTagIndex;
typedef ui32 TStateId;
typedef ui16 TLiteralId;
typedef ui16 TRuleId;

static inline TRuleId NoneRuleId() {
    return Max<TRuleId>();
}

typedef NSorted::TSimpleMap<TTagId, TTagIndex> TTagIdToIndex;

} // NRemorph

Y_DECLARE_OUT_SPEC(inline, NRemorph::TTagIdToIndex, out, tii) {
    out << "{";
    bool first = true;
    for (NRemorph::TTagIdToIndex::const_iterator iTag = tii.begin(); iTag != tii.end(); ++iTag) {
        if (first) {
            first = false;
        } else {
            out << ",";
        }
        out << iTag->first << "->" << iTag->second;
    }
    out << "}";
}
