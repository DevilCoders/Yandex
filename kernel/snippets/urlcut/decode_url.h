#pragma once

#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/doccodes.h>
#include <util/generic/string.h>

namespace NUrlCutter {
    TUtf16String DecodeUrlHostAndPath(const TString& encodedUrl, ELanguage docLang = LANG_RUS, ECharset docEnc = CODES_UNKNOWN);
    TUtf16String DecodeUrlHost(const TString& host);
    TUtf16String DecodeUrlPath(const TString& encodedUrlPath, ELanguage docLang = LANG_RUS, ECharset docEnc = CODES_UNKNOWN);
}
