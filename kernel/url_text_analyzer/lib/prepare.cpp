#include "prepare.h"

#include <contrib/libs/libidn/lib/idna.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/charset/utf8.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NUta {
    TUtf16String PrepareUrl(TStringBuf url, bool cutWWW, bool cutZeroDomain, bool processWinCode) {
        // FIXME: hack for urls from Gemini duplicates table
        if (Y_UNLIKELY(url.size() > 4 && url[3] == '@')) {
            url = url.Tail(4);
        }

        url = CutSchemePrefix(url);
        if (cutWWW) {
            url = CutWWWPrefix(url);
        }

        TString result = ToString(GetHost(url));

        // begin with host preparation
        char* punyDecoded = nullptr;
        if (IDNA_SUCCESS == idna_to_unicode_8z8z(result.data(), &punyDecoded, 0)) {
            result = punyDecoded;
        }
        if (punyDecoded != nullptr) {
            free(punyDecoded);
        }
        if (cutZeroDomain) {
            TStringBuf hostNoDomain, zeroDomain;
            TStringBuf(result).RSplit('.', hostNoDomain, zeroDomain);
            if (Y_UNLIKELY(hostNoDomain.EndsWith(".com"))) {
                result.resize(hostNoDomain.size() - 4lu);
            } else {
                result.resize(hostNoDomain.size());
            }
        }

        // path and params processing
        TStringBuf pathAndParams = GetPathAndQuery(url, true);
        TVector<char> tmpBuf(CgiUnescapeBufLen(pathAndParams.size()));
        if (Y_UNLIKELY(processWinCode)) {
            const char* unescEnd = UrlUnescape(tmpBuf.data(), pathAndParams);
            auto srcBeg = reinterpret_cast<const unsigned char*>(tmpBuf.cbegin());
            const auto srcEnd = reinterpret_cast<const unsigned char*>(unescEnd);
            wchar32 rune;
            size_t runeLen;
            while (srcBeg < srcEnd && SafeReadUTF8Char(rune, runeLen, srcBeg, srcEnd) == RECODE_OK) {
                srcBeg += runeLen;
            }
            const auto utf8End = reinterpret_cast<const char*>(srcBeg);
            if (tmpBuf.data() < utf8End) {
                result.append(tmpBuf.data(), utf8End);
            }
            if (utf8End < unescEnd) {
                TString recoded;
                Recode(CODES_WIN, CODES_UTF8, TStringBuf(utf8End, unescEnd), recoded);
                result.append(recoded);
            }
        } else {
            result.append(tmpBuf.data(), UrlUnescape(tmpBuf.data(), pathAndParams));
        }

        return UTF8ToWide<true>(result);
    }

}
