#include "segmentator.h"

namespace NSegm {
namespace NPrivate {

union TReadabilityMode {
    TReadabilityMode()
        : SpecialModes()
    {
        MayHaveArticle = 1;
        WeightClasses = 1;
        StripUnlikelys = 1;
        CleanJunk = 1;
    }

    ui32 SpecialModes = 0;

    struct {
        ui32 MayHaveArticle :1;
        ui32 WeightClasses :1;
        ui32 StripUnlikelys :1;
        ui32 CleanJunk :1;
    };

    bool Ease() {
        if (!MayHaveArticle)
            return false;

        SpecialModes >>= 1;
        return true;
    }
};


inline i16 GetClassWeight(const TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return 0;

    i16 w = 0;
    if (node->Props.BlockMarkers.NegativeClass)
        w -= 25;
    if (node->Props.BlockMarkers.PositiveClass)
        w += 25;
    if (node->Props.BlockMarkers.NegativeId)
        w -= 25;
    if (node->Props.BlockMarkers.PositiveId)
        w += 25;
    return w;
}

inline i16 GetTagWeight(const TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return 0;

    switch (node->Props.Tag) {
    default:
        return 0;
    case HT_DIV:
        return 5;
    case HT_PRE: case HT_TD: case HT_BLOCKQUOTE:
        return 3;
    case HT_ADDRESS: case HT_OL: case HT_UL: case HT_DL: case HT_DD: case HT_DT: case HT_LI: case HT_FORM:
        return -3;
    case HT_H1: case HT_H2: case HT_H3: case HT_H4: case HT_H5: case HT_H6: case HT_TH:
        return -5;
    }
}

inline bool CheckNodeCleanedOut(TDocNode* node) {
    for(node = node->GetBlock(); node; node = node->Parent)
        if (node->Props.CleanedOut)
            return true;

    return false;
}

static ui32 CountLinks(TDocNode* node) {
    if (IsA<DNT_LINK>(node))
        return 1;

    if (!IsA<DNT_BLOCK>(node))
        return 0;

    ui32 nlinks = 0;
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        nlinks += CountLinks(&*it);

    return nlinks;
}

template <ETagBreakLevel Level>
static ui32 CountBreaks(TDocNode* node) {
    if (IsA<DNT_BREAK>(node))
        return node->Props.Level >= Level;

    if (!Iterateable(node))
        return 0;

    ui32 n = 0;
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        n += CountBreaks<Level>(&*it);

    return n + (IsA<DNT_BLOCK>(node) && GetBreakLevel(node->Props.Tag) >= Level);
}

static ui32 CountInputs(TDocNode* node) {
    if (IsA<DNT_INPUT>(node))
        return 1;

    if (!Iterateable(node))
        return 0;

    ui32 n = 0;
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        n += CountInputs(&*it);

    return n;
}


static ui32 CountCommas(TDocNode* node, ui32 stopafter = -1, int level = 0) {
    if (IsA<DNT_TEXT>(node))
        return node->Props.TextMarkers.CommasInText;

    if (!Iterateable(node))
        return 0;

    ui32 ncommas = 0;
    for (TDocNode::iterator it = node->Begin(); it != node->End() && ncommas < stopafter; ++it)
        ncommas += CountCommas(&*it, stopafter - ncommas, level + 1);

    if (level != 0)
        return ncommas;

    return stopafter == (ui32)-1 ? ncommas : ncommas >= stopafter;
}

static ui32 CountTextLength(TDocNode* node, ui32 stopafter = -1, int level = 0) {
    if (CheckNodeCleanedOut(node))
        return 0;

    if (IsA<DNT_TEXT>(node))
        return node->Props.TextMarkers.SymbolsInText;

    if (!Iterateable(node))
        return 0;

    ui32 len = 0;
    for (TDocNode::iterator it = node->Begin(); it != node->End() && len < stopafter; ++it)
        len += CountTextLength(&*it, stopafter - len, level + 1);

    if (level != 0)
        return len;

    return stopafter == (ui32)-1 ? len : len >= stopafter;
}

static float CountLinkDensity(TDocNode* node) {
    ui32 len = CountTextLength(node);
    ui32 nlinks = CountLinks(node);
    return len ? (float)nlinks / len : Min<float>(nlinks, 1);
}

static bool ResetReadability(TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return true;

    node->ResetReadability();

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        ResetReadability(&*it);

    return true;
}

static void StripUnlikelyCandidates(TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return;

    if (node->Parent && !node->Props.BlockMarkers.PossibleCandidateCSS && node->Props.BlockMarkers.UnlikelyCandidateCSS) {
        node->Props.CleanedOut = 1;
        return;
    }

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        StripUnlikelyCandidates(&*it);
}

static bool HasBlockChildren(const TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return false;

    for (TDocNode::const_iterator it = node->Begin(); it != node->End(); ++it) {
        if (IsA<DNT_BLOCK>(&*it) && (GetBreakLevel(it->Props.Tag) >= TBL_PP || HasBlockChildren(&*it)))
            return true;
    }

    return false;
}

inline void InitCandidate(TDocNode* node, TReadabilityMode mode) {
    if (!node || node->Props.IsACandidate)
        return;

    node->Props.IsACandidate = true;
    node->Props.ContentScore = GetTagWeight(node) + GetClassWeight(node) * mode.WeightClasses;
}

static bool MarkCandidates(TDocNode* node, TReadabilityMode mode) {
    if (!IsA<DNT_BLOCK>(node))
        return false;

    bool hasblocks = false;
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        hasblocks |= MarkCandidates(&*it, mode);

    if (!node->Parent || hasblocks || node->Props.CleanedOut || !CountTextLength(node, 25))
        return true;

    TDocNode* parent = node->Parent;
    TDocNode* gparent = parent->Parent;

    InitCandidate(parent, mode);
    InitCandidate(gparent, mode);

    float score = 1 + CountCommas(node) + CountTextLength(node, 300)/100.;

    parent->Props.ContentScore += score;

    if (gparent)
        gparent->Props.ContentScore += score/2;

    return true;
}

static void NormContentScore(TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return;

    if (node->Props.IsACandidate)
        node->Props.ContentScore *= (1 - CountLinkDensity(node));

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        NormContentScore(&*it);
}

inline TDocNode* ChooseBestTop(TDocNode* a, TDocNode* b) {
    if (!a || !a->Props.IsACandidate)
        return b && b->Props.IsACandidate ? b : nullptr;
    if (!b || !b->Props.IsACandidate)
        return a;
    return b->Props.ContentScore > a->Props.ContentScore ? b : a;
}

static TDocNode* SelectTopCandidate(TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return nullptr;

    TDocNode* top = ChooseBestTop(node, nullptr);

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        top = ChooseBestTop(top, SelectTopCandidate(&*it));

    return top;
}

static void SelectContentSiblings(TDocNode* topcontent) {
    if (!topcontent || !topcontent->Parent)
        return;

    float thr = Min(10., topcontent->Props.ContentScore * 0.2);

    TDocNode* parent = topcontent->Parent;
    for (TDocNode::iterator it = parent->Begin(); it != parent->End(); ++it) {
        if (it->Props.IsACandidate && it->Props.ContentScore >= thr) {
            it->Props.AddedToContent = true;
            continue;
        }

        if (it->Props.Class == topcontent->Props.Class)
            it->Props.ContentScore += topcontent->Props.ContentScore * 0.2;

        if (!HasBlockChildren(&*it)) {
            float linkdens = CountLinkDensity(&*it);
            bool islong = CountTextLength(&*it, 80);
            bool hascommas = CountCommas(&*it, 1);

            if ((islong && linkdens < 0.25) || (!linkdens && hascommas))
                it->Props.AddedToContent = true;
        }
    }
}

inline bool LooksLikeAGoodArticle(TDocNode* topcontentparent) {
    if (!topcontentparent)
        return false;

    ui32 len = 0;
    for (TDocNode::iterator it = topcontentparent->Begin(); it != topcontentparent->End(); ++it)
        if (IsA<DNT_BLOCK>(&*it) && it->Props.AddedToContent)
            len += CountTextLength(&*it);

    return len >= 250;
}

static void FillExcluded(TSpans& excluded, const TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return;

    if (node->Props.CleanedOut) {
        excluded.push_back(TSpan(node->NodeStart.SentAligned(), node->NodeEnd.SentAligned()));
        return;
    }

    for (TDocNode::const_iterator it = node->Begin(); it != node->End(); ++it)
        FillExcluded(excluded, &*it);
}

static void AppendSpan(TSpans& spans, TAlignedPosting beg, TAlignedPosting end) {
    if (beg >= end)
        return;

    if (!spans.empty() && spans.back().ContainsInEnd(beg))
        spans.back().End = end;
    else
        spans.push_back(TSpan(beg, end));
}

static void GetContent(TSpans& spans, TDocNode* topcontentparent) {
    if (!topcontentparent)
        return;

    TSpans included;
    TSpans excluded;

    for (TDocNode::iterator it = topcontentparent->Begin(); it != topcontentparent->End(); ++it) {
        if (!IsA<DNT_BLOCK>(&*it) || !it->Props.AddedToContent)
            continue;

        included.push_back(TSpan(it->NodeStart.SentAligned(), it->NodeEnd.SentAligned()));
        FillExcluded(excluded, &*it);
    }

    TSpans::const_iterator et = excluded.begin();
    for (TSpans::const_iterator it = included.begin(); it != included.end(); ++it) {
        while (et != excluded.end() && et->Begin < it->Begin)
            ++et;

        if (et == excluded.end() || et->Begin >= it->End) {
            AppendSpan(spans, it->Begin, it->End);
            continue;
        }

        AppendSpan(spans, it->Begin, et->Begin);
        for (TSpans::const_iterator eet = et; eet != excluded.end() && eet->Begin < it->End; (et = eet), ++eet)
            AppendSpan(spans, et->End, eet->Begin);

        AppendSpan(spans, et->End, it->Begin);
    }
}

static void CleanJunk(TDocNode* node) {
    if (!IsA<DNT_BLOCK>(node))
        return;

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        if (!IsA<DNT_BLOCK>(&*it))
            continue;

        CleanJunk(&*it);

        if (it->Props.AddedToContent)
            continue;

        i16 w = GetClassWeight(&*it);
        if (it->Props.ContentScore + GetClassWeight(&*it) < 0) {
            it->Props.CleanedOut = true;
            continue;
        }

        if (CountCommas(&*it, 10))
            continue;

        float d = CountLinkDensity(&*it);
        if ((w < 25 && d > 0.2) || (w >= 25 && d > 0.5) ) {
            it->Props.CleanedOut = true;
            continue;
        }

        ui32 inputs = CountInputs(&*it);
        ui32 parabreaks = CountBreaks<TBL_PP>(&*it);

        if (inputs * 3 > parabreaks)
            it->Props.CleanedOut = true;
    }
}

static TDocNode* GrabReadableArticle(TDocNode* root, TReadabilityMode mode) {
    TDocNode* parent = nullptr;

    do {
        if (mode.StripUnlikelys)
            StripUnlikelyCandidates(root);

        MarkCandidates(root, mode);
        NormContentScore(root);
        TDocNode* node = SelectTopCandidate(root);

        if (!node) {
            node = root;
            InitCandidate(node, mode);
        }

        SelectContentSiblings(node);
        parent = node ? node->Parent : nullptr;

        if (mode.CleanJunk)
            CleanJunk(parent);
    } while(!LooksLikeAGoodArticle(parent) && ResetReadability(root) && mode.Ease());

    if (!mode.MayHaveArticle)
        return nullptr;

    return parent;
}

void TSegmentator::GrabArticle() {
    using namespace NPrivate;

    TSpans mains;
    GetContent(mains, GrabReadableArticle(Root, TReadabilityMode()));

    for (TSpans::const_iterator it = mains.begin(); it != mains.end(); ++it) {
        if (it->NoSents())
            continue;

        Ctx->ReadabilitySpans.push_back(*it);
    }
}

}
}
