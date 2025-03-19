#pragma once

#include "wordnode.h"

#include <util/generic/fwd.h>
#include <util/system/defaults.h>

class TPure;
class TRichRequestNode;
class TLanguageContext;
class TPureWithCachedWord;

ui64 ExactWordFreq(const TPureWithCachedWord& pure, const TWordNode& node);
ui64 CommonWordFreq(const TPureWithCachedWord& pure, const TWordNode& node, bool exactLemma);
double GetUpDivLoFreq(const TPure& pure, const TWordNode& node);
void LoadFreq(const TPure& pure, TWordNode& node);
void LoadFreq(const TPure& pure, TRichRequestNode& node, bool force = false);
i64 GetRevFreq(const TPure& pure, const TUtf16String& word, const TLanguageContext& lang, TFormType ftype = fGeneral);
i64 GetRevFreqForNode(const TPure& pure, const TWordNode& node);
