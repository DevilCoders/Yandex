#include "greenurl.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/uri/uri.h>
#include <kernel/url_tools/idna_decode.h>

#include <util/charset/utf8.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/string_utils/quote/quote.h>

static void FillDesktopGreenUrl(const TString& rawUrlWithScheme, const TStringBuf& prettyUrl, NSc::TValue& serpData) {
    serpData["path"]["items"][0]["text"] = CutWWWPrefix(GetOnlyHost(prettyUrl));
    serpData["path"]["items"][0]["url"] = GetSchemeHostAndPort(rawUrlWithScheme, false);

    TStringBuf pathAndQuery = GetPathAndQuery(rawUrlWithScheme);
    if (pathAndQuery.empty()) {
        // This case never actually happens because pathAndQuery is at least "/".
        serpData["path"]["items"][0]["url"] = rawUrlWithScheme;
        return;
    }

    TStringBuf head = pathAndQuery.NextTok('?');
    TStringBuf tail;

    while (tail.empty() && !head.empty()) {
        tail = head.RNextTok('/');
    }

    TString tailString(tail);
    CGIUnescape(tailString);

    if (tailString.empty() || !IsUtf(tailString)) {
        serpData["path"]["items"][0]["url"] = rawUrlWithScheme;
        return;
    }

    SubstGlobal(tailString, '_', ' ');
    serpData["path"]["items"][1]["text"] = tailString;
    serpData["path"]["items"][1]["url"] = rawUrlWithScheme;
}

static void FillMobileGreenUrl(const TString& rawUrlWithScheme, const TStringBuf& prettyUrl, NSc::TValue& serpData) {
    serpData["path"]["items"][0]["text"] = CutWWWPrefix(GetOnlyHost(prettyUrl));
    serpData["path"]["items"][0]["url"] = rawUrlWithScheme;
}

void NFacts::FillUrlAndGreenUrl(const TStringBuf& rawUrl, NSc::TValue& serpData, bool isTouch) {
    if (!rawUrl) {
        return;
    }
    TString rawUrlWithScheme = AddSchemePrefix(TString(rawUrl));
    TString prettyUrl;

    if (!IDNAUrlToUtf8(rawUrlWithScheme, prettyUrl, false)) {
        prettyUrl = rawUrlWithScheme;
    }

    serpData["url"] = rawUrlWithScheme;

    if (isTouch) {
        FillMobileGreenUrl(rawUrlWithScheme, prettyUrl, serpData);
    } else {
        FillDesktopGreenUrl(rawUrlWithScheme, prettyUrl, serpData);
    }
}
