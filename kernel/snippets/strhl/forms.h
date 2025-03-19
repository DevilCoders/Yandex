#pragma once

#include <util/string/vector.h>

#include "goodwrds.h"

typedef enum {LGM_NONE, LGM_UNDERLINE, LGM_CURLY} LemmataGlueMode;

class HashSet;
void Request2Lemms(const char* request, TVector<TString> &lemms, LemmataGlueMode gluelemms, const HashSet* stopWords = nullptr);
void Request2Forms(const char* request, TVector<TString> &words, bool collectMinusWords);

TString GetGoodForms(const char* request, bool collectMinusWords);
TString GetGoodWords(const char* request, const HashSet* stopWords = nullptr);
TString GetGoodWordsADVQ(const char* request, const HashSet* stopWords = nullptr);
TString GetGoodWordsK(const char* request, const HashSet* stopWords = nullptr);

size_t LemmCount(const TInlineHighlighter& ih, const char* InputText, size_t InputTextLength);
