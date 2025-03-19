#pragma once

#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/smartcut/cutparam.h>
#include <kernel/snippets/smartcut/multi_length_cut.h>
#include <kernel/snippets/idl/enums.h>
#include <util/generic/string.h>

class TRichRequestNode;
class TWordFilter;
class TInlineHighlighter;
struct TPaintingOptions;
struct TZonedString;

namespace NSnippets {

class TQueryy;
class TConfig;
class TSnipTitle;
class TSentsMatchInfo;
class TWordSpanLen;
class TPassageReplyData;

class TSmartCutOptions {
public:
    TCutParams CutParams = TCutParams::Symbol();
    const TWordFilter* StopWordsFilter = nullptr;
    bool MaximizeLen = false;
    float Threshold = 2.0f / 3.0f;
    bool AddEllipsisToShortText = true;
    bool CutLastWord = false;
    const wchar16* HilightMark = nullptr;
public:
    TSmartCutOptions();
    TSmartCutOptions(const TConfig& cfg);
};

void SmartCut(
    TUtf16String& text,
    const TInlineHighlighter& ih,
    float maxLen,
    const TSmartCutOptions& options);

TMultiCutResult SmartCutWithExt(
    const TUtf16String& text,
    const TInlineHighlighter& ih,
    float maxLen,
    float maxExtLen,
    const TSmartCutOptions& options);

TUtf16String CutSnip(
    const TUtf16String& raw,
    const TConfig& cfg,
    const TQueryy& query,
    float maxLen);

TUtf16String CutSnip(
    const TUtf16String& raw,
    const TConfig& cfg,
    const TQueryy& query,
    float maxLen,
    float maxLenForLPFactors,
    const TSnipTitle* title,
    const TWordSpanLen& wordSpanLen);

TMultiCutResult CutSnipWithExt(
    const TUtf16String& raw,
    const TReplaceContext& ctx,
    float maxLen,
    float maxExtLen,
    float maxLenForLPFactors,
    const TSnipTitle* title,
    const TWordSpanLen& wordSpanLen);

TUtf16String CutSnippet(
    const TUtf16String& text,
    const TRichRequestNode* richtreeRoot,
    size_t maxSymbols);

void FillReadMoreLineInReply(
    TPassageReplyData& replyData,
    TVector<TZonedString>& paintedFragments,
    const TVector<TUtf16String>& extendedSnipVec,
    const TInlineHighlighter& highlighter,
    const TPaintingOptions& paintingOptions,
    const TConfig& cfg,
    ISnippetsCallback* explainCallback);
} // namespace NSnippets
