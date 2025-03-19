#include "multi_length_cut.h"
#include "consts.h"

#include <util/generic/strbuf.h>

namespace NSnippets {

    TMultiCutResult::TMultiCutResult(const TUtf16String& shortStr, const TUtf16String& longStr)
        : Short(shortStr)
        , Long(longStr)
    {
        if (Short.size() <= Long.size()) {
            TWtringBuf longBuf(Long);
            TWtringBuf shortBuf(Short);
            longBuf.ChopSuffix(BOUNDARY_ELLIPSIS);
            shortBuf.ChopSuffix(BOUNDARY_ELLIPSIS);
            CharCountDifference = longBuf.size() - shortBuf.size();
            CommonPrefixLen = shortBuf.size();
        }
    }

    TMultiCutResult::operator bool() const {
        return !!Short;
    }
}
