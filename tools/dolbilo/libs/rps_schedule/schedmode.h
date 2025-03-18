#pragma once

#include <util/generic/serialized_enum.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

enum ESchedMode {
    SM_AT_UNIX = 0      /* "at_unix"   */,
    SM_STEP             /* "step" */,
    SM_LINE             /* "line" */,
    SM_CONST            /* "const" */
};

