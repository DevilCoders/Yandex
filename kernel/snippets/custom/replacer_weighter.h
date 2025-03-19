#pragma once

#include <kernel/snippets/config/enums.h>
#include <kernel/snippets/markers/markers.h>
#include <kernel/snippets/smartcut/multi_length_cut.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/list.h>
#include <util/generic/string.h>

namespace NSnippets
{

class TReplaceManager;
class TReplaceContext;

enum ESubReplacerResult
{
    ReplacementFound,
    ReplacementNotYetFound,
    AbortReplacing
};

struct TSpecSnippetCandidate
{
    TSnipTitle Title;
    TUtf16String Text;
    TMultiCutResult ExtendedText;
    TString Source;
    double PrecalcedWeight;
    int Priority;
    bool DontReplace;

    void SetExtendedText(const TMultiCutResult& result) {
        ExtendedText = result;
        Text = result.Short;
    }

    TSpecSnippetCandidate()
        : Title()
        , PrecalcedWeight(INVALID_SNIP_WEIGHT)
        , Priority(.0)
        , DontReplace(false)
    {
    }
};

enum ECompareAlgo
{
    COMPARE_MXNET,
    COMPARE_HILITE,
    COMPARE_RANDOM
};

class TReplacerWeighter
{
    const TReplaceContext& Context;
    const ELanguage Lang;

    bool LangMatch(const TUtf16String& text);
    int TitleUglinessLevel(const TSnipTitle& title);

public:
    TReplacerWeighter(const TReplaceContext& context, ELanguage lang)
        : Context(context)
        , Lang(lang)
    {
    }

    bool HasBetterTitle(const TSpecSnippetCandidate* chosenOne);
    ESubReplacerResult PerformReplace(TReplaceManager& manager, const EMarker marker,
                                      const TList<TSpecSnippetCandidate>& candidates,
                                      ECompareAlgo algo);
};

}
