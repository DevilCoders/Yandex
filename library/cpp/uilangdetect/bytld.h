#pragma once

#include <util/generic/string.h>
#include <library/cpp/langs/langs.h>

TString TLDByHost(const TString& host);
ELanguage LanguageByTLD(const TString& tld);
