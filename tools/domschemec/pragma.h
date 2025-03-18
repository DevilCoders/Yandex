#pragma once

#include <util/generic/string.h>

struct TPragmas {
    bool ProcessSvnKeywords = false;
};

bool UpdatePragmas(TPragmas&, const TStringBuf&, bool);
