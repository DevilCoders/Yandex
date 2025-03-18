#pragma once

#include <util/generic/string.h>
#include <library/cpp/regex/pcre/regexp.h>

namespace NCssConfig {
    TRegExMatch* CompileRegexp(const TString& reg_str);
}
