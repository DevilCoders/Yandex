#include "url.h"

#include <library/cpp/charset/recyr.hh>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/string/cast.h>
#include <util/string/cstriter.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/util.h>
#include <util/system/defaults.h>
#include <util/system/maxlen.h>

namespace {
    const char szHexDigits[] = "0123456789ABCDEF";

    template <class I1, class I2, class I3, class I4>
    inline I1 EncodeRFC1738Impl(I1 f, I2 l, I3 b, I4 e) noexcept {
        while (b != e && f != l) {
            const unsigned char c = *b;

            if (c == '+') {
                *f++ = '%';
                if (f == l) {
                    return f;
                }
                *f++ = '2';
                if (f == l) {
                    return f;
                }
                *f++ = '0';
            } else if (c <= 0x20 || c >= 0x80) {
                *f++ = '%';
                if (f == l) {
                    return f;
                }
                *f++ = szHexDigits[c >> 4];
                if (f == l) {
                    return f;
                }
                *f++ = szHexDigits[c & 15];
            } else {
                *f++ = c;
            }

            ++b;
        }

        return f;
    }

    template <typename TChar>
    inline bool UrlEncodeRFC1738AndCutLastSlash(TChar* buf, size_t len, const char* url) noexcept {
        TChar* const ret = EncodeRFC1738Impl(buf, buf + len, url, TCStringEndIterator());

        if (ret == buf + len) {
            return false;
        } else {
            if (ret[-1] == '/') {
                ret[-1] = 0;
            } else {
                *ret = 0;
            }

            return true;
        }
    }
}

char* EncodeRFC1738(char* buf, size_t len, const char* url, size_t urllen) noexcept {
    char* ret = EncodeRFC1738Impl(buf, buf + len, url, url + urllen);

    if (ret != buf + len) {
        *ret++ = 0;
    }

    return ret;
}

bool NormalizeUrl(char* pszRes, size_t nResBufSize, const char* pszUrl, size_t pszUrlSize) {
    Y_ASSERT(nResBufSize);
    pszUrlSize = pszUrlSize == TString::npos ? strlen(pszUrl) : pszUrlSize;
    size_t nBufSize = pszUrlSize * 3 + 10;
    TTempBuf buffer(nBufSize);
    char* const szUrlUtf = buffer.Data();
    size_t nRead, nWrite;
    Recode(CODES_WIN, CODES_UTF8, pszUrl, szUrlUtf, pszUrlSize, nBufSize, nRead, nWrite);
    szUrlUtf[nWrite] = 0;
    return UrlEncodeRFC1738AndCutLastSlash(pszRes, nResBufSize, szUrlUtf);
}

TString NormalizeUrl(const TStringBuf& url) {
    size_t len = url.size() + 1;
    while (true) {
        TTempBuf buffer(len);
        if (NormalizeUrl(buffer.Data(), len, url.data(), url.size()))
            return buffer.Data();
        len <<= 1;
    }
    ythrow yexception() << "impossible";
}

TUtf16String NormalizeUrl(const TWtringBuf& url) {
    const size_t buflen = url.size() * sizeof(wchar16);
    TTempBuf buffer(buflen + 1);
    char* const data = buffer.Data();
    size_t read = 0, written = 0;
    RecodeFromUnicode(CODES_UTF8, url.data(), data, url.size(), buflen, read, written);
    data[written] = 0;

    size_t len = written + 1;
    while (true) {
        TArrayHolder<wchar16> p(new wchar16[len]);
        if (UrlEncodeRFC1738AndCutLastSlash(p.Get(), len, data))
            return p.Get();
        len <<= 1;
    }
    ythrow yexception() << "impossible";
}

TString StrongNormalizeUrl(const TStringBuf& url) {
    TString result(url);
    result = ToLower(result, csYandex);
    TStringBuf tmp = result;
    if (!tmp.AfterPrefix("http://", tmp)) {
        (void)tmp.AfterPrefix("https://", tmp);
    }
    tmp.AfterPrefix("www.", tmp);
    TTempBuf normalizedUrl(FULLURL_MAX + 7);
    if (!::NormalizeUrl(normalizedUrl.Data(), FULLURL_MAX, tmp.data(), tmp.size()))
        return TString(tmp);
    else
        return normalizedUrl.Data();
}
