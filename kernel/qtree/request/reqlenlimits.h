#pragma once

#include <util/system/defaults.h>

struct TLengthLimits {
    TLengthLimits();
    TLengthLimits(size_t maxNumberOfTokens, size_t maxNumberOfChars);

    size_t MaxNumberOfTokens;
    size_t MaxNumberOfChars;
};
