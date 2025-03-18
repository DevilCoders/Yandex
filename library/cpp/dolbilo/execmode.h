#pragma once

#include <util/generic/serialized_enum.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

enum EExecMode {
    EM_BY_PLAN = 0 /* "plan"   */,
    EM_DEVASTATE /* "devastate" */,
    EM_FUCKUP = EM_DEVASTATE /* "fuckup" */, // FIXME(mvel) remove this item
    EM_FINGER /* "finger" */,
    EM_BY_BINARY_SEARCH /* "binary" */
};


inline TString GetExecModeHelp() {
    return " * plan      - run requests with delays corresponding to precalculated plan.\n"
           " * devastate - run next request as soon as previous finished, thus ignoring plan,\n"
           "               fire the same requests in all the requester fibers.\n"
           " * finger    - same as devastate but distribute requests between fibers so that\n"
           "               every request is fired exactly once.\n"
           " * binary    - try to find the rps which would yield error rate as close to the\n"
           "               predefined one as possible.\n";
}
