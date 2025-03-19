#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NTurbo {

TString GetSaasKeyFromUrl(TStringBuf url, bool isListing);

TString RemoveTrailingSlash(TStringBuf url);
TString NormalizeSaasKey(TStringBuf saasKey);
TString CanonizeSaasKey(TStringBuf saasKey);
TString CanonizeHost(TStringBuf host);

TString CanonizeUrlForSaasDeprecatedLegacy(TStringBuf url);

} // namespace NTurboPages
