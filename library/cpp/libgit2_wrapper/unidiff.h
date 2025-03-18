#pragma once

#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace NLibgit2 {
    /**
     * Calculates and dumps diff in unified format.
     */
    void UnifiedDiff(TStringBuf l,
                     TStringBuf r,
                     ui32 context,
                     IOutputStream& out,
                     bool colored = false,
                     bool ignoreWhitespace = false,
                     bool ignoreWhitespaceChange = false,
                     bool ignoreWhitespaceEOL = false);

}
