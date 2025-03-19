#pragma once

#include "smart.h"

#include <util/generic/string.h>

struct TIsUrlSmartTransliterateResult: public TIsUrlSmartResult {
    bool IsExact;
    bool IsTranslit;
    TString Translit;

    TIsUrlSmartTransliterateResult();
    TIsUrlSmartTransliterateResult& operator=(const TIsUrlSmartResult& res);
};

TIsUrlSmartTransliterateResult IsUrlSmartTranslit(const TString& req, int flags, bool tryNoTranslit = true);
