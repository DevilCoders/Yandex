#pragma once

#include <library/cpp/uri/common.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

struct TIsUrlResult {
    bool IsUrl;
    TString ResUrl;
    TString ResUrlEncoded; //percent-encoded url
    bool IsIDNA;
    TString ResUrlIDNA; //punycode-encoded url

    TIsUrlResult();
    TIsUrlResult(bool isUrl, const TString& link, const TString& encoded, bool IsIDNA = false, const TString& idnaLink = TString());
};

/*
 * flags are from NUri::TField::EFlags
 *  they are used as an argument to NUri::TUri::Print that is used to output
 *  ResUrl, ResUrlEncoded, ResUrlIDNA
 */
TIsUrlResult IsUrl(TStringBuf req, int flags);
