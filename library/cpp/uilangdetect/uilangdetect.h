#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/misc/httpreqdata.h>

using TLangVector = TVector<ELanguage>;

ELanguage DetectUserLanguage(const TServerRequestData& rd, const TLangVector& goodLanguages = TLangVector());

ELanguage DetectUserLanguage(const TString& d, const TString& l10n, const TString& mc, TStringBuf al, const TLangVector& gl = TLangVector());

TMaybe<ELanguage> TryDetectUserLanguage(const TServerRequestData& rd, const TLangVector& goodLanguages = TLangVector());

TMaybe<ELanguage> TryDetectUserLanguage(const TString& d, const TString& l10n, const TString& mc, TStringBuf al, const TLangVector& gl = TLangVector());
