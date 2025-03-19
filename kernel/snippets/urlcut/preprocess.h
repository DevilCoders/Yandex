#pragma once

#include <util/generic/string.h>

namespace NUrlCutter {
    TString CutUrlPrefix(TString& url);
    void PreprocessUrl(TString& url);
}
