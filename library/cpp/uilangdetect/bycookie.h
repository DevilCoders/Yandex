#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <library/cpp/langs/langs.h>

TString GetCookieFromHeader(TStringBuf header, TStringBuf cookieName);

ELanguage LanguageByMyCookie(const TString& myCookie);
