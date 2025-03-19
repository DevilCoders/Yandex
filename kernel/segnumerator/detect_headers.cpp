#include "segmentator.h"

namespace NSegm {
namespace NPrivate {

static TDocNode * FrontText(TDocNode * leaf) {
    if (IsA<DNT_TEXT>(leaf))
        return leaf;

    if (!IsA<DNT_LINK>(leaf) || leaf->ListEmpty())
        return nullptr;

    for (TDocNode::iterator it = leaf->Begin(); it != leaf->End(); ++it)
        if (IsA<DNT_TEXT>(&*it))
            return &*it;

    return nullptr;
}

inline bool HasBreak(TDocNode * link, bool front = true) {
    return !link->ListEmpty() && IsA<DNT_BREAK>(front ? link->Front() : link->Back());
}

const ui32 MaxHeaderWords = 35;

enum EHeadDetSt {
    HDS_NONE = 0, HDS_NOT_HEADER, HDS_MAY_BE_HEADER, HDS_HEADER, HDS_STRICT_HEADER
};

bool IsHeader(EHeadDetSt state)
{
    return state == HDS_HEADER || state == HDS_STRICT_HEADER;
}

static EHeadDetSt AnalizeLink(TDocNode *, TDocNode *&, ui32&);
static EHeadDetSt AnalizeBlock(TDocNode *, TDocNode *&, THeaderSpans&);

static EHeadDetSt AnalizeLeaf(TDocNode * node, TDocNode *& nextText, ui32& nWords) {
    Y_VERIFY(!IsA<DNT_BLOCK>(node), " ");

    if (IsA<DNT_INPUT>(node))
        return HDS_NOT_HEADER;

    if (IsA<DNT_BREAK>(node))
        return node->Props.Level > TBL_BR ? HDS_NOT_HEADER : HDS_MAY_BE_HEADER;

    if (IsA<DNT_TEXT>(node)) {
        nWords += node->Props.NWords;

        if (node->Props.NWords > MaxHeaderWords)
            return HDS_NOT_HEADER;

        TDocNode * ont = nextText;
        nextText = node;

        if ((ont && node->Props.BoldDistance.CanBeHeader(ont->Props.BoldDistance))
                        || (!ont && node->Props.BoldDistance.CanBeHeader()))
            return HDS_HEADER;

        return HDS_MAY_BE_HEADER;
    }

    if (IsA<DNT_LINK>(node))
        return AnalizeLink(node, nextText, nWords);

    Y_FAIL(" ");
    return HDS_NONE;
}

static EHeadDetSt AnalizeLink(TDocNode * link, TDocNode *& nextText, ui32& nWords) {
    EHeadDetSt stat = HDS_NONE;
    ui32 myNWords = 0;

    for (TDocNode::reverse_iterator it = link->RBegin(); it != link->REnd(); ++it) {
        TDocNode * node = &*it;

        EHeadDetSt s = AnalizeLeaf(node, nextText, myNWords);

        if (HDS_NOT_HEADER == s) {
            stat = HDS_NOT_HEADER;
            break;
        }

        if ((!stat && HDS_HEADER == s) || HDS_NOT_HEADER == s)
            stat = s;

        if (myNWords > MaxHeaderWords) {
            stat = HDS_NOT_HEADER;
            break;
        }
    }

    nWords += myNWords;
    return stat ? stat : HDS_MAY_BE_HEADER;
}

static EHeadDetSt AnalizeSingleBlock(TDocNode * node, TDocNode *& nextText, EHeadDetSt selfStat,
        THeaderSpans& headerZones) {
    Y_VERIFY(IsA<DNT_BLOCK>(node) && IsA<DNT_BLOCK>(node->Front()), " ");
    EHeadDetSt st = AnalizeBlock(node->Front(), nextText, headerZones);

    if (HDS_MAY_BE_HEADER != st)
        selfStat = HDS_NOT_HEADER;

    if (IsHeader(selfStat) && node->SentBegin() < node->SentEnd()) {
        while (!headerZones.empty() && node->ContainsSent(headerZones.back().Begin.Sent()))
            headerZones.pop_back();

        THeaderSpan newSpan(node->NodeStart.SentAligned(), node->NodeEnd.SentAligned());
        newSpan.IsStrictHeader = selfStat == HDS_STRICT_HEADER;
        headerZones.push_back(newSpan);
    }

    return selfStat ? selfStat : HDS_MAY_BE_HEADER;
}

static EHeadDetSt AnalizeBlock(TDocNode * block, TDocNode *& nextnextText, THeaderSpans& headerZones) {
    Y_VERIFY(IsA<DNT_BLOCK>(block), " ");
    EHeadDetSt selfStat =
            IsHeaderTag(block->Props.Tag) || block->Props.BlockMarkers.HeaderCSS ? HDS_STRICT_HEADER : HDS_NONE;
    ui32 nWords = 0;

    if (block->HasSingleChild() && IsA<DNT_BLOCK>(block->Front()))
        return AnalizeSingleBlock(block, nextnextText, selfStat, headerZones);

    for (TDocNode::reverse_iterator it = block->RBegin(); it != block->REnd(); ++it) {
        TDocNode * node = &*it;
        TDocNode * prevprev = TNodeAccessor::GetNextNext(it, block->REnd());
        TDocNode * prev = TNodeAccessor::GetNext(it, block->REnd());
        TDocNode * next = TNodeAccessor::GetPrev(it, block->RBegin());
        TDocNode * nextnext = TNodeAccessor::GetPrevPrev(it, block->RBegin());
        TDocNode * nextText = nextnextText;

        if (IsA<DNT_BLOCK>(node)) {
            selfStat = HDS_NOT_HEADER;
            AnalizeBlock(node, nextText, headerZones);
            continue;
        }

        EHeadDetSt st = AnalizeLeaf(node, nextnextText, nWords); //replaces nextnextText

        if (nWords > MaxHeaderWords)
            selfStat = HDS_NOT_HEADER;

        if (HDS_NOT_HEADER == st)
            selfStat = HDS_NOT_HEADER;

        TDocNode * frontText = FrontText(node);

        if (IsHeader(st)
                && (!next
                        || IsA<DNT_BREAK>(next)
                        || (IsA<DNT_LINK>(next)
                                && (HasBreak(next) || ( next->ListEmpty() && (!nextnext || IsA<DNT_BREAK>(nextnext))))
                           )
                   )
                && (!prev
                        || IsA<DNT_BREAK>(prev)
                        || (IsA<DNT_LINK>(prev)
                                && (HasBreak(prev, false) || (prev->ListEmpty() && (!prevprev || IsA<DNT_BREAK>(prevprev))))
                           )
                   )
                && (!nextText
                        || (frontText && frontText->Props.BoldDistance.CanBeHeader(nextText->Props.BoldDistance))
                   )
           )
        {
            if (node->SentBegin() < node->SentEnd()) {
                THeaderSpan newSpan(node->NodeStart.SentAligned(), node->NodeEnd.SentAligned());
                newSpan.IsStrictHeader = st == HDS_STRICT_HEADER;
                headerZones.push_back(newSpan);
            }
        }
    }

    if (IsHeader(selfStat) && nWords) {
        while (!headerZones.empty() && block->ContainsSent(headerZones.back().Begin.Sent()))
            headerZones.pop_back();
        THeaderSpan newSpan(block->NodeStart.SentAligned(), block->NodeEnd.SentAligned());
        newSpan.IsStrictHeader = selfStat == HDS_STRICT_HEADER;
        headerZones.push_back(newSpan);
    }

    return selfStat ? selfStat : HDS_MAY_BE_HEADER;
}

void TSegmentator::DetectHeaders() {
    TDocNode * nextText = nullptr;
    AnalizeBlock(Root, nextText, Ctx->HeaderSpans);
    std::reverse(Ctx->HeaderSpans.begin(), Ctx->HeaderSpans.end());
    for (THeaderSpans::const_iterator it = Ctx->HeaderSpans.begin(); it != Ctx->HeaderSpans.end(); ++it)
    {
        if (it->IsStrictHeader) {
            Ctx->StrictHeaderSpans.push_back(*it);
        }
    }
}

}
}
