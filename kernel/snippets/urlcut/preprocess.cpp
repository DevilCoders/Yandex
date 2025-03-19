#include "preprocess.h"
#include "consts.h"

#include <util/generic/strbuf.h>
#include <util/string/subst.h>

namespace NUrlCutter {
    static constexpr TStringBuf HTTP_PREFIX = "http://";
    static constexpr TStringBuf HTTPS_PREFIX = "https://";
    static constexpr TStringBuf AUTO_PREFIX = "//";
    static constexpr TStringBuf WWW_PREFIX = "www.";

    void PreprocessUrl(TString& url) {
        SubstGlobal(url, UGLY_AJAX_CGI_SHARP_EXCLAMATION, SHARP_EXCLAMATION);
        SubstGlobal(url, UGLY_AJAX_AMP_SHARP_EXCLAMATION, SHARP_EXCLAMATION);
        SubstGlobal(url, U_RTL_OVERRIDE, EMPTY_CHARS);
    }

    TString CutUrlPrefix(TString& url) {
        size_t prefixLength = 0;
        if (url.StartsWith(HTTP_PREFIX)) {
            prefixLength = HTTP_PREFIX.size();
        } else if (url.StartsWith(HTTPS_PREFIX)) {
            prefixLength = HTTPS_PREFIX.size();
        } else if (url.StartsWith(AUTO_PREFIX)) {
            prefixLength = AUTO_PREFIX.size();
        }
        if (TStringBuf(url).SubStr(prefixLength).StartsWith(WWW_PREFIX)) {
            prefixLength += WWW_PREFIX.size();
        }
        const TString prefix = url.substr(0, prefixLength);
        url.erase(0, prefixLength);
        return prefix;
    }
}
