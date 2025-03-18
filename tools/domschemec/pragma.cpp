#include "pragma.h"
#include <util/generic/strbuf.h>

bool UpdatePragmas(TPragmas& p, const TStringBuf& s, bool on) {
    if (s == "process_svn_keywords") {
        p.ProcessSvnKeywords = on;
        return true;
    }
    return false;
}
