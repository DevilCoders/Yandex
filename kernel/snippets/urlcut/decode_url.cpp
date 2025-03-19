#include "decode_url.h"
#include "consts.h"

#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/unicode/punycode/punycode.h>
#include <util/generic/ptr.h>
#include <library/cpp/string_utils/quote/quote.h>

namespace NUrlCutter {
    TUtf16String DecodeUrlHostAndPath(const TString& encodedUrl, ELanguage docLang, ECharset docEnc) {
        TString cutUrl = encodedUrl;
        size_t idx = cutUrl.find(PATH_SEP_CHAR);
        if (idx == TString::npos) {
            idx = cutUrl.size();
        }
        TUtf16String decodedHost = DecodeUrlHost(cutUrl.substr(0, idx));
        TUtf16String decodedPath = DecodeUrlPath(cutUrl.substr(idx), docLang, docEnc);
        return decodedHost + decodedPath;
    }

    TUtf16String DecodeUrlHost(const TString& host) {
        return ForcePunycodeToHostName(host);
    }

    TUtf16String DecodeUrlPath(const TString& encodedUrlPath, ELanguage docLang, ECharset docEnc) {
        TString url = CGIUnescapeRet(encodedUrlPath);
        if (IsUtf(url)) {
            return UTF8ToWide(url);
        } else {
            if (docEnc != CODES_YANDEX && ValidCodepage(docEnc) && SingleByteCodepage(docEnc)) {
                return CharToWide(url, docEnc);
            } else if (ScriptByLanguage(docLang) == SCRIPT_CYRILLIC) {
                return CharToWide(url, CODES_WIN);
            } else if (docLang == LANG_TUR) {
                return CharToWide(url, CODES_WINDOWS_1254);
            } else if (ScriptByLanguage(docLang) == SCRIPT_ARABIC) {
                return CharToWide(url, CODES_WINDOWS_1256);
            }
            // Worst case: nothing was detected.

            // Without check, this will crash in debug mode on url
            // 'http://vksounds.ru/music/%CA%E8%EF%F0+Ivi+Adamou++/0' with docLang = 'eng'
            // and gives the following stack trace: https://paste.yandex-team.ru/137853
            // This seems to be very rare case, but happens in production when document
            // language is detected incorrectly.
            // See https://st.yandex-team.ru/SEARCH-2225#1469554169000 for details.
            // See also https://st.yandex-team.ru/SEARCH-6106
            if (IsStringASCII(url.begin(), url.end())) {
                return ASCIIToWide(url);
            }
            return TUtf16String();
        }
    }
}
