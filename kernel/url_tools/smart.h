#pragma once

#include "url_tools.h"

#include <util/generic/string.h>

struct TIsUrlSmartResult: public TIsUrlResult {
    bool IsStraight;

    TIsUrlSmartResult();
    TIsUrlSmartResult(const TIsUrlResult& isUrlResult, bool isStraight);
};

TIsUrlSmartResult IsUrlSmart(TString req, int flags);
