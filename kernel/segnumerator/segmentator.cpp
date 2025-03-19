#include "segmentator.h"
#include "structfinder.h"

#include <kernel/segmentator/structs/classification.h>
#include <kernel/segmentator/main_header_impl.h>
#include <kernel/segmentator/main_content_impl.h>
#include <kernel/segmentator/structs/merge.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/hash.h>

namespace NSegm {
namespace NPrivate {

typedef TList<TSegmentSpan, TPoolAllocator> TSSpanPooledList;
typedef TSegmentSpanAccessor<TSSpanPooledList> TSSpanPooledListAccessor;


class THashTokenHandler: public ITokenHandler {
public:
    THashTokenHandler(THashValues& tokenHashes)
    : TokenHashes(tokenHashes)
    {}
    void OnToken(const TWideToken& w, size_t, NLP_TYPE type) override {
        if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT) {
            TokenHashes.push_back(ComputeHash(TWtringBuf(w.Token, w.Leng)));
        }
    }
private:
    THashValues& TokenHashes;
};

void TokenizeStr(const TStringBuf& str, THashValues& tokenHashes) {
    THashTokenHandler handler(tokenHashes);
    TNlpTokenizer tokenizer(handler);
    tokenizer.Tokenize(str.data(), str.size());
}

size_t CommonElements(const THashValues& lhs, const THashValues& rhs) {
    size_t leftPosition = 0;
    size_t rightPosition = 0;
    size_t result = 0;

    while (leftPosition < lhs.size() && rightPosition < rhs.size()) {
        size_t left = lhs[leftPosition];
        size_t right = rhs[rightPosition];

        if (left < right) {
            ++leftPosition;
        } else if (left > right) {
            ++rightPosition;
        } else {
            ++leftPosition;
            ++rightPosition;
            ++result;
        }
    }

    return result;
}

TDocContext FillDocContext(TSegmentSpans& sp,
    const THashTokens& s,
    const TStringBuf& metaDescrStr)
{
    TDocContext ctx;
    THashValues title;
    THashValues footer;
    THashValues metaDescr;

    s.CollectHashes(title, TTokenTraits::TitleFilter(true));

    for (TSegmentSpans::iterator it = sp.begin(); it != sp.end(); ++it) {
        if (STP_FOOTER == it->Type)
            s.CollectHashes(footer, *it);
    }

    TokenizeStr(metaDescrStr, metaDescr);

    Sort(title.begin(), title.end());
    Sort(footer.begin(), footer.end());
    Sort(metaDescr.begin(), metaDescr.end());

    ctx.TitleWords = title.size();
    ctx.FooterWords = footer.size();
    ctx.MetaDescrWords = metaDescr.size();

    THashValues segtxt;
    for (TSegmentSpans::iterator it = sp.begin(); it != sp.end(); ++it) {
        if (!it->MainContentCandidate())
            continue;

        ++ctx.GoodSegs;
        ctx.Words += it->Words;
        ctx.MaxWords = Max<ui32>(ctx.MaxWords, it->Words);

        segtxt.clear();
        s.CollectHashes(segtxt, *it);
        Sort(segtxt.begin(), segtxt.end());

        it->TitleWords = CommonElements(title, segtxt);
        it->FooterWords = CommonElements(footer, segtxt);
        it->MetaDescrWords = CommonElements(metaDescr, segtxt);
    }

    return ctx;
}

void TSegmentator::ProcessTextEnd(const TNumerStat& stat) {
    CheckTextAndReset();
    Y_VERIFY(Ctx->BlockStack.size(), " ");

    while (Ctx->BlockStack.size() > 1)
        CloseBlockNode(stat, Ctx->BlockStack.back().Tag);

    Root->NodeEnd = stat.TokenPos.Pos;

    Y_VERIFY(Root->NodeEnd >= Root->NodeStart, "%u < %u", (TPosting)Root->NodeEnd, (TPosting)Root->NodeStart);

    TStructFinder lfinder(Ctx);
    lfinder.FindAndFillCtx(Root);

    ProcessTree();
    DetectHeaders();
    GrabArticle();

    Linearize(Root);

    TSSpanPooledList lst(&Ctx->Pool);

    MakeSegmentList(lst, Ctx->Segments);

    CheckIncludes(lst, Ctx->HeaderSpans, SetHeader);

    PeriodicMerge<TSSpanPooledListAccessor> (lst);

    Merge<THeaderToHeaderMergeDesider, TSSpanPooledListAccessor> (lst);
    Merge<TNodeToNodeMergeDesider<2, BST_ITEM> , TSSpanPooledListAccessor> (lst);
    Merge<THeaderToNodeMergeDesider<2> , TSSpanPooledListAccessor> (lst);

    Classify(lst);

    Merge<TNodeToNodeTypedMergeDesider<2> , TSSpanPooledListAccessor> (lst);

    Classify(lst);

    Y_VERIFY(Ctx->SegmentSpans.empty(), " ");
    Ctx->SegmentSpans.insert(Ctx->SegmentSpans.end(), lst.begin(), lst.end());

    DocContext = FillDocContext(Ctx->SegmentSpans, Ctx->Tokens, MetaDescription);

    Ctx->ArticleSpans = FindArticles(Ctx->HeaderSpans, Ctx->SegmentSpans);
    CheckIsIncluded(Ctx->SegmentSpans, Ctx->ArticleSpans, SetInArticle);
    CheckIsIncluded(Ctx->SegmentSpans, Ctx->ReadabilitySpans, SetInReadabilitySpans);

    if (SkipMainContent)
        return;

    Ctx->MainContentSpans = FindMainContentSpans(DocContext, Ctx->HeaderSpans, Ctx->SegmentSpans, &Ctx->Pool);

    CheckIsIncluded(Ctx->SegmentSpans, Ctx->MainContentSpans, SetInMainContentNews);

    if (!Ctx->MainContentSpans.empty() && !SkipMainHeader)
        Ctx->MainHeaderSpans = FindMainHeaderSpans(
                        DocContext, Ctx->HeaderSpans, Ctx->SegmentSpans, Ctx->MainContentSpans, Ctx->ArticleSpans);

    CheckIncludes(Ctx->SegmentSpans, Ctx->MainHeaderSpans, SetHasMainHeaderNews);

    for (TSegmentSpans::iterator it = Ctx->SegmentSpans.begin(); it != Ctx->SegmentSpans.end(); ++it) {
        if (it->InMainContentNews) {
            it->MainContentFront = true;

            if (it != Ctx->SegmentSpans.begin()) {
                TSegmentSpans::iterator sit = it - 1;

                if (sit->HasHeader && TBlockDist(sit->LastBlock, it->FirstBlock).StructDist() < 5)
                    sit->MainContentFrontAdjoining = true;
            }

            break;
        }
    }

    for (TSegmentSpans::reverse_iterator it = Ctx->SegmentSpans.rbegin(); it != Ctx->SegmentSpans.rend(); ++it) {
        if (it->InMainContentNews) {
            it->MainContentBack = true;

            if (it != Ctx->SegmentSpans.rbegin()) {
                TSegmentSpans::reverse_iterator sit = it - 1;

                if (TBlockDist(sit->FirstBlock, it->FirstBlock).StructDist() < 5 && !sit->CommentsCSS
                                && EqualToOneOf(sit->Type, STP_CONTENT, STP_MENU, STP_LINKS, STP_AUX))
                    sit->MainContentBackAdjoining = true;
            }

            break;
        }
    }

}

void TSegmentator::Linearize(TDocNode* node) {
    Y_VERIFY(IsA<DNT_BLOCK> (node), " ");
    OpenSegmentNode(node, true);

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        TDocNode * child = &*it;

        if (IsA<DNT_BLOCK> (child)) {
            Linearize(child);
            OpenSegmentNode(child, false);
        } else
            CurrentSegment->Add(child);
    }

    CloseSegmentNode(node, true);
}
}
}
