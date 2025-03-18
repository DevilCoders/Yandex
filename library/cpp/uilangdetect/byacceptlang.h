#pragma once

#include "uilangdetect.h"

ELanguage LanguageByAcceptLanguage(TStringBuf accLang, ELanguage defaultLang, const TLangVector& goodLanguages);
TLangVector ReadLanguages(TStringBuf langString, const TString& delimiter = ",");
