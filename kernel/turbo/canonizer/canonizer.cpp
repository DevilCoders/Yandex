#include "canonizer.h"

#include <util/string/builder.h>
#include <util/string/subst.h>
#include <library/cpp/uri/uri.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/string_utils/quote/quote.h>


namespace {
    TStringBuf HTTP_SCHEME = "http://";
    TStringBuf HTTPS_SCHEME = "https://";

    TString CanonizePath(TStringBuf path) {
        TStringBuilder result;
        while (path) {
            if (path.StartsWith('/')) {
                path = path.SubStr(1);
                result << '/';
                continue;
            } else {
                size_t pos = path.find('/');
                TString token { path.substr(0, pos) };
                path = path.substr(pos);
                UrlUnescape(token);
                Quote(token, "!\"$&'()*,:;<=>@[\\]`{|}~");
                SubstGlobal(token, "+", "%20"); // Quote is actually QuotePlus, but we need just Quote
                SubstGlobal(token, "%2B", "+");
                result << token;
            }
        }
        return result;
    }
}


namespace NTurbo {

TString GetSaasKeyFromUrl(TStringBuf url, bool isListing) {
    if (isListing) {
        return TString("listing") + TString{url};
    }
    return TString{url}; // Note: implementation will be chanhed in future
}

TString RemoveTrailingSlash(TStringBuf url) {
    TStringBuf path, query, fragment;
    SeparateUrlFromQueryAndFragment(url, path, query, fragment);
    path.ChopSuffix("/");
    TStringBuilder result;
    result << path;
    if (query) {
        result << '?' << query;
    }
    if (fragment) {
        result << '#' << fragment;
    }
    return result;
}

TString NormalizeSaasKey(TStringBuf saasKey) {
    TStringBuf scheme = GetSchemePrefix(saasKey);
    if (scheme != HTTP_SCHEME && scheme != HTTPS_SCHEME) {
        return TString(saasKey);
    }
    TString normalized;
    NUri::TUri uri;
    if (uri.Parse(saasKey, NUri::TFeature::FeaturesRecommended | NUri::TFeature::FeatureNoRelPath) == NUri::TState::ParsedOK) {
        uri.Print(normalized);
    } else {
        normalized = saasKey;
    }
    return normalized;
}

TString CanonizeSaasKey(TStringBuf saasKey) {
    TStringBuf scheme = GetSchemePrefix(saasKey);
    if (scheme != HTTP_SCHEME && scheme != HTTPS_SCHEME) {
        return TString(saasKey);
    }
    TString normalized = NormalizeSaasKey(saasKey);
    TStringBuf host;
    TStringBuf path;
    SplitUrlToHostAndPath(normalized, host, path);

    TStringBuf query, fragment;
    SeparateUrlFromQueryAndFragment(path, path, query, fragment);
    TStringBuilder result;
    result << HTTPS_SCHEME << CanonizeHost(host) << CanonizePath(path);
    if (query) {
        result << '?' << query;
    }
    if (fragment) {
        result << '#' << fragment;
    }
    return result;
}

TString CanonizeHost(TStringBuf url) {
    auto host = GetOnlyHost(url);
    size_t oldLen = 0;
    do {
        oldLen = host.length();
        host = CutWWWPrefix(host);
        host = CutMPrefix(host);
    } while (oldLen > host.length());
    TString lowerHost(host);
    lowerHost.to_lower();

    return lowerHost;
}

TString CanonizeUrlForSaasDeprecatedLegacy(TStringBuf url) {
    TString result = TString{CutMPrefix(CutWWWPrefix(CutHttpPrefix(url)))};
    SubstGlobal(result, "?utm_referrer=https%3A%2F%2Fzen.yandex.com&", "?");
    SubstGlobal(result, "?utm_referrer=https%3A%2F%2Fzen.yandex.com", "");
    SubstGlobal(result, "&utm_referrer=https%3A%2F%2Fzen.yandex.com", "");
    return result;
}

} // namespace NTurboPages
