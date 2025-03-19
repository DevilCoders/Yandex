#include "url2docid.h"
#include <util/string/cast.h>
#include <util/string/util.h>

namespace NUrl2DocId {
    static bool HasUpperCase(TStringBuf str) {
        static str_spn upper{"A-Z", true};
        return upper.brk(str.begin(), str.end()) != str.end();
    }

    TStringBuf GetUrlWithLowerCaseHost(TStringBuf mainUrl, TString& str) {
        const size_t prefixSize = GetSchemePrefixSize(mainUrl);
        const TStringBuf host = GetHost(mainUrl.SubStr(prefixSize));
        if (HasUpperCase(host)) {
            str.assign(mainUrl);
            str.to_lower(prefixSize, host.size());
            return str;
        } else {
            return mainUrl;
        }
    }

    TString GetUrlWithLowerCaseHost(TStringBuf mainUrl) {
        TString buf;
        return ToString(GetUrlWithLowerCaseHost(mainUrl, buf));
    }
}


TString& Url2DocIdRaw(TStringBuf url, TString& hashbuf) {
    using namespace NUrlId;
    return Hash2StrZ(UrlHash64(url), hashbuf);
}

TDocHandle Url2DocIdRaw(TStringBuf url) {
    return TDocHandle(UrlHash64(url));
}

TDocHandle Url2DocId(TStringBuf url) {
    return Url2DocIdRaw(DoPrepareUrlForMultilanguage(url));
}

TString& Url2DocId(TStringBuf url, TStringBuf lang, TString& langbuf, TString& hashbuf) {
    using namespace NUrlId;
    return Url2DocIdRaw(DoEncodeMultilanguageSearchUrl(lang, url, langbuf), hashbuf);
}

TString Url2DocIdSimple(TStringBuf url, TStringBuf lang) {
    TString tmp, result;
    return Url2DocId(url, lang, tmp, result);
}

TString& Url2DocId(TStringBuf url, TString& urlHashBuf, TString& hostCaseBuf, bool host2lower) {
    using namespace NUrlId;
    return Url2DocIdRaw(DoPrepareUrlForMultilanguage(host2lower ? NUrl2DocId::GetUrlWithLowerCaseHost(url, hostCaseBuf) : url), urlHashBuf);
}

TString& Url2DocId(TStringBuf url, TString& urlHashBuf, bool host2lower) {
    using namespace NUrlId;
    if (host2lower) {
        TString hostCaseBuf;
        return Url2DocId(url, urlHashBuf, hostCaseBuf, host2lower);
    } else {
        return Url2DocIdRaw(DoPrepareUrlForMultilanguage(url), urlHashBuf);
    }
}

TString Url2DocIdSimple(TStringBuf url) {
    TString result;
    return Url2DocId(url, result);
}

TString Url2DocIdSimpleHost2Lower(TStringBuf url) {
    TString result;
    return Url2DocId(url, result, true);
}
