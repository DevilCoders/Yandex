#include "reqlenlimits.h"

TLengthLimits::TLengthLimits()
    : MaxNumberOfTokens(64)
    , MaxNumberOfChars(500)
{
}

TLengthLimits::TLengthLimits(size_t maxNumberOfTokens, size_t maxNumberOfChars)
    : MaxNumberOfTokens(maxNumberOfTokens)
    , MaxNumberOfChars(maxNumberOfChars)
{
}
