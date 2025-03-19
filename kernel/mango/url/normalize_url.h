#pragma once

#include <util/generic/string.h>
#include <util/string/vector.h>
#include <library/cpp/uri/http_url.h>

namespace NMango {
    TString NormalizeUrl(const TString& url, const TString& replUrl);
    void FilterCgiParamsByList(THttpURL &url, bool isWhiteList, const TVector<TString> &list, bool keepParamsWithoutValue, bool sort);
    static inline TString NormalizeUrl(const TString& url) {
        return NormalizeUrl(url, url);
    }
    void NormalizePureTextLinks(TVector<TString>& urls);
}
