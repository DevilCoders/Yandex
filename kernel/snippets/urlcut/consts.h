#pragma once

#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NUrlCutter {
    static const TUtf16String DOT = u".";

    static constexpr char CGI_SEP_CHAR = '?';
    static constexpr char PATH_SEP_CHAR = '/';

    static constexpr wchar16 CGI_SEP = wchar16(CGI_SEP_CHAR);
    static constexpr wchar16 PATH_SEP = wchar16(PATH_SEP_CHAR);
    static constexpr wchar16 SOFT_SIGN = 0x44C;
    static constexpr wchar16 HARD_SIGN = 0x44A;
    static constexpr wchar16 RTL_OVERRIDE = 0x202E;

    static const TString UGLY_AJAX_SHARP_EXCLAMATION("_escaped_fragment_=");
    static const TString UGLY_AJAX_CGI_SHARP_EXCLAMATION = TString(CGI_SEP_CHAR) + UGLY_AJAX_SHARP_EXCLAMATION;
    static const TString UGLY_AJAX_AMP_SHARP_EXCLAMATION = TString('&') + UGLY_AJAX_SHARP_EXCLAMATION;
    static const TString SHARP_EXCLAMATION("#!");
    static const TString U_RTL_OVERRIDE = WideToUTF8(&RTL_OVERRIDE, 1);
    static const TString EMPTY_CHARS("");

    static const TUtf16String Ellipsis(wchar16(0x2026));
    static const TUtf16String W_CGI_SEP(CGI_SEP);

    static constexpr i32 MAX_W = 10000000;

    extern const THashMap<TUtf16String, i32> GrayList;
    extern const THashSet<TUtf16String> StopWords;

}
