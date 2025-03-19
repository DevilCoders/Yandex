#pragma once

#include "segment.h"
#include "token.h"

#include <kernel/segnumerator/utils/storer.h>

#include <library/cpp/wordpos/wordpos.h>

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/face/parsface.h>

#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/html/spec/tags.h>

#include <library/cpp/numerator/numerate.h>

#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

namespace NSegm {
namespace NPrivate {

struct TImgCtx {
    TString Src;
    TString Alt;

    void Clear() {
        Src.clear();
        Alt.clear();
    }

    bool Empty() const {
        return !Src && !Alt;
    }
};

struct TSegContext {
    TMemoryPool Pool;

    TUrlInfo OwnerInfo;

    TSegmentList Segments;
    TSegmentSpans SegmentSpans;

    THeaderSpans HeaderSpans;
    THeaderSpans StrictHeaderSpans;
    TMainHeaderSpans MainHeaderSpans;

    TSpans ReadabilitySpans;
    TArticleSpans ArticleSpans;
    TMainContentSpans MainContentSpans;

    TTypedSpans ListSpans;
    TTypedSpans ListItemSpans;
    TSpans TableSpans;
    TSpans TableRowSpans;
    TSpans TableCellSpans;

    THashTokens Tokens; // kostyl, through cheap event storer
    TImgCtx ImgCtx;     // kostyl, through event storer :: on media

    TVector<TTagItem> BlockStack;

    TUtf16String CurrentText;

    TSegContext()
        : Pool(500000)
    {
    }

    void Clear() {
        CurrentText.clear();
        BlockStack.clear();
        ImgCtx.Clear();
        Tokens.clear();
        TableCellSpans.clear();
        TableRowSpans.clear();
        TableSpans.clear();
        ListItemSpans.clear();
        ListSpans.clear();
        MainContentSpans.clear();
        ArticleSpans.clear();
        ReadabilitySpans.clear();
        MainHeaderSpans.clear();
        HeaderSpans.clear();
        StrictHeaderSpans.clear();
        SegmentSpans.clear();
        Segments.Clear();
        OwnerInfo.Clear();
        Pool.ClearKeepFirstChunk();
    }
};

class TSegmentator : TNonCopyable {
protected:
    static const ui32 MaxTreeDepth = 257;
    static const ui32 MaxNodeCount = 1 << 14;

    enum {
        ExpectAttrNone = 0,
        /*hrefs*/ExpectAttrAncor = 1, ExpectAttrArea = 2,
        /*header markers, path discriminators*/ExpectAttrBlock = 3,
        /*header markers in hrefs*/ExpectAttrInlineBlock = 4,
        /*header markers*/ExpectAttrSpan = 5,
        /*alt, src*/ExpectAttrImg = 6,
    };

    TSegContext* Ctx = nullptr;
    IStorer* Storer = nullptr;
    TDocNode* Root = nullptr;
    ui32 TotalWordCount = 0;
    ui32 NodeCount = 0;
    bool ZeroWeight = 0;
    HT_TAG IgnoreContentUntilClosed = HT_any;

    TDocNode* CurrentNode = nullptr;

    TBoldDistance BoldDistance = TBoldDistance::New();
    ui16 SpanCount = 0; //for bold span detection

    TLinkMarkers LinkMarkers = TLinkMarkers::New();
    ui8 AttributeExpectation = ExpectAttrNone;

    TSegmentNode* CurrentSegment = nullptr;

    bool SkipMainHeader = 0;
    bool SkipMainContent = 0;

    TDocContext DocContext;
    TStringBuf MetaDescription;

    bool Html5Parser = 0;

public:
    void SetStorer(IStorer* callback) {
        Storer = callback;
    }

    // Use html5Parser = true only with library/cpp/html/html5 parser running before numerator
    void SetHtml5Parser(bool html5Parser) {
        Html5Parser = html5Parser;
    }

    void SetSegContext(TSegContext* ctx) {
        Ctx = ctx;
        Ctx->Clear();

        Root = MakeBlock(Ctx->Pool, TAlignedPosting(1,1), nullptr, HT_any);
        CurrentNode = Root;
        Ctx->BlockStack.push_back(TTagItem(HT_any, Root));
        NodeCount = 1;
    }

    void InitOwnerInfo(const char* url, const TOwnerCanonizer* c) {
        Ctx->OwnerInfo.SetCanonizer(c);
        Ctx->OwnerInfo.SetUrl(url);
    }

    void SetSkipMainContent() {
        SkipMainContent = true;
    }

    void SetSkipMainHeader() {
        SkipMainHeader = true;
    }

    const TString& GetUrl() const {
        return Ctx->OwnerInfo.RawUrl;
    }

    const THashTokens& GetTokens() const {
        return Ctx->Tokens;
    }

    const TUrlInfo& GetOwnerInfo() const {
        return Ctx->OwnerInfo;
    }

    TUrlInfo& GetOwnerInfo() {
        return Ctx->OwnerInfo;
    }

    const THeaderSpans& GetHeaderSpans() const {
        return Ctx->HeaderSpans;
    }

    THeaderSpans& GetHeaderSpans() {
        return Ctx->HeaderSpans;
    }

    const THeaderSpans& GetStrictHeaderSpans() const {
        return Ctx->StrictHeaderSpans;
    }

    THeaderSpans& GetStrictHeaderSpans() {
        return Ctx->StrictHeaderSpans;
    }

    const TSegmentSpans& GetSegmentSpans() const {
        return Ctx->SegmentSpans;
    }

    TSegmentSpans& GetSegmentSpans() {
        return Ctx->SegmentSpans;
    }

    const TMainHeaderSpans& GetMainHeaderSpans() const {
        return Ctx->MainHeaderSpans;
    }

    TMainHeaderSpans& GetMainHeaderSpans() {
        return Ctx->MainHeaderSpans;
    }

    const TMainContentSpans& GetMainContentSpans() const {
        return Ctx->MainContentSpans;
    }

    TMainContentSpans& GetMainContentSpans() {
        return Ctx->MainContentSpans;
    }

    const TArticleSpans& GetArticleSpans() const {
        return Ctx->ArticleSpans;
    }

    TArticleSpans& GetArticleSpans() {
        return Ctx->ArticleSpans;
    }

    const TTypedSpans& GetListSpans() const {
        return Ctx->ListSpans;
    }

    TTypedSpans& GetListSpans() {
        return Ctx->ListSpans;
    }

    const TTypedSpans& GetListItemSpans() const {
        return Ctx->ListItemSpans;
    }

    TTypedSpans& GetListItemSpans() {
        return Ctx->ListItemSpans;
    }

    const TSpans& GetTableSpans() const {
        return Ctx->TableSpans;
    }

    TSpans& GetTableSpans() {
        return Ctx->TableSpans;
    }

    const TSpans& GetTableRowSpans() const {
        return Ctx->TableRowSpans;
    }

    TSpans& GetTableRowSpans() {
        return Ctx->TableRowSpans;
    }

    const TSpans& GetTableCellSpans() const {
        return Ctx->TableCellSpans;
    }

    TSpans& GetTableCellSpans() {
        return Ctx->TableCellSpans;
    }

    const TDocContext& GetDocContext() const {
        return DocContext;
    }

    TDocContext& GetDocContext() {
        return DocContext;
    }

    bool ProcessToken(const TWideToken& w, const TNumerStat& stat) {
        bool title = IsTitleText(stat);
        if (!title && IgnoreContent())
            return false;

        if (IsAsciiEmojiPart(TWtringBuf(w.Token, w.Leng))) {
            // previous version of the tokenizer has directed this text to ProcessSpaces instead
            if (!title)
                Ctx->CurrentText.append(w.Token, w.Leng);
            return true;
        }

        Ctx->Tokens.push_back(THashToken(ComputeHash(TWtringBuf(w.Token, w.Leng)), stat.TokenPos.DocLength()));

        if (title) {
            Ctx->Tokens.back().Title = true;
            Root->NodeStart = TAlignedPosting(stat.TokenPos.Pos).NextSent();
        } else {
            TDocNode* last = CurrentTextNode();

            if (!last || last->Props.BoldDistance != BoldDistance) {
                if (MaxNodeCount <= NodeCount)
                    return true;
                NodeCount++;
                last = MakeText(Ctx->Pool, stat.TokenPos.Pos, stat.TokenPos.Pos, CurrentNode, 0, BoldDistance);
            }

            Ctx->Tokens.back().Link = IsInsideLink();
            last->Props.NWords += 1;
            TotalWordCount += 1;
            last->NodeEnd = TAlignedPosting(stat.TokenPos.Pos).NextWord();
            Ctx->CurrentText.append(w.Token, w.Leng);
        }

        return true;
    }

    void CheckTextAndReset() {
        TDocNode* last = CurrentTextNode();
        if (last && !!Ctx->CurrentText)
            last->Props.TextMarkers.CheckText(Ctx->CurrentText.data(), Ctx->CurrentText.size());
        Ctx->CurrentText.clear();
    }

    void ProcessSpaces(TBreakType, const wchar16* s, unsigned n, const TNumerStat& stat) {
        if (!IsTitleText(stat) && !IgnoreContent())
            Ctx->CurrentText.append(s, n);
    }

    void BeforeProcessDiscardInput(const TNumerStat& stat) {
        ZeroWeight = WEIGHT_ZERO == stat.CurWeight;
    }

    void ProcessDiscardInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat& stat) {
        ProcessInput(chunk, ze, stat);
    }

    void AfterProcessDiscardInput() {
    }

    void ProcessTextEnd(const TNumerStat& stat);

    bool IsTitleText(const TNumerStat& stat) {
        return WEIGHT_BEST == stat.CurWeight;
    }

    bool IsUsefulBodyText(const TNumerStat& stat) {
        return !IgnoreContent() && !IsTitleText(stat);
    }

    bool IsInsideLink() const {
        return IsA<DNT_LINK> (CurrentNode);
    }

    bool IgnoreContent() const {
        return IgnoreElement() || ZeroWeight;
    }

protected:
    bool IgnoreElement() const {
        return IgnoreContentUntilClosed != HT_any;
    }

    void CheckIgnoreContent(HT_TAG tag, bool close) {
        if (close && IgnoreContentUntilClosed == tag) {
            IgnoreContentUntilClosed = HT_any;
        }

        if (!close && EqualToOneOf(tag, /*HT_TITLE, */HT_TEXTAREA, HT_BUTTON, HT_XMP))
            IgnoreContentUntilClosed = tag;
    }

    bool IgnoredHierarchy() {
        return IsA<DNT_LINK> (CurrentNode) || Ctx->BlockStack.size() >= MaxTreeDepth;
    }

    TDocNode* CurrentBlockNode() {
        return IsA<DNT_BLOCK> (CurrentNode) ? CurrentNode : CurrentNode->Parent;
    }
    TDocNode* CurrentLinkNode() {
        return IsA<DNT_LINK> (CurrentNode) ? CurrentNode : nullptr;
    }
    TDocNode* CurrentBreakNode() {
        return GetLastNode<DNT_BREAK> (CurrentNode);
    }
    TDocNode* CurrentTextNode() {
        return GetLastNode<DNT_TEXT> (CurrentNode);
    }
    TDocNode* CurrentInputNode() {
        return GetLastNode<DNT_INPUT> (CurrentNode);
    }

    void ProcessTree();

    void ProcessInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat&);
    void ProcessMarkup(const THtmlChunk& htev, const TNumerStat&);
    void ProcessAttribute(const TAttrEntry attr, const TNumerStat&);

    void ProcessLinkAttribute (ETagAttributeType type, const char* attr, ui32 len, const TNumerStat&);
    void ProcessBlockAttribute(ETagAttributeType type, const char* attr, ui32 len, const TNumerStat&);
    void ProcessAncorAttribute(ETagAttributeType type, const char* attr, ui32 len, const TNumerStat&);
    void ProcessSpanAttribute (ETagAttributeType type, const char* attr, ui32 len, const TNumerStat&);
    void ProcessImgAttribute  (ETagAttributeType type, const char* attr, ui32 len, const TNumerStat&);
    void ProcessInlineBlockAttribute(ETagAttributeType type, const char* attr, ui32 len, const TNumerStat&);

    void OpenBlockNode(const TNumerStat&, HT_TAG tag, bool closeLast = false);
    void CloseBlockNode(const TNumerStat&, HT_TAG tag);

    void OpenLinkNode(const TNumerStat&, const TString& url);
    void CloseLinkNode(const TNumerStat&);

    void AddBreak(const TNumerStat&, HT_TAG tag);
    void AddInput(const TNumerStat&);

    void FlushAttributes(const TNumerStat&);

    void DetectHeaders();

    void GrabArticle();

    void OpenSegmentNode(TDocNode* node, bool include) {
        Y_VERIFY(IsA<DNT_BLOCK> (node), " ");
        CloseSegmentNode(node, !include);
        CurrentSegment = new (Ctx->Pool) TSegmentNode();
        CurrentSegment->SetFrontBlock(node, include);
    }

    void CloseSegmentNode(TDocNode* node, bool include) {
        Y_VERIFY(IsA<DNT_BLOCK> (node), " ");

        if (CurrentSegment) {
            if (!CurrentSegment->IsEmpty())
                Ctx->Segments.PushBack(CurrentSegment);

            CurrentSegment->SetBackBlock(node, include);
            CurrentSegment = nullptr;
        }
    }

    void Linearize(TDocNode* node);


};

}
}
