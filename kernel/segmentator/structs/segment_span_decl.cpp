#include "segment_span_decl.h"

namespace NSegm {

TBlockInfo TBlockInfo::Make(bool included) {
    TBlockInfo i;
    i.Depth = 0;
    i.Included = included;
    return i;
}

TBlockInfo TBlockInfo::Make(ui32 blocks, ui32 pars, ui32 items, bool hassuper, bool included) {
    TBlockInfo i;
    i.Blocks = Min(blocks, (ui32) MaxBlocks);
    i.Paragraphs = Min(pars, (ui32) MaxParagraphs);
    i.Items = Min(items, (ui32) MaxItems);
    i.HasSuperItem = hassuper;
    i.Included = included;
    return i;
}

void TBlockInfo::Add(EBlockStepType w) {
    switch (w) {
    default:
        break;
    case BST_SUPITEM:
        HasSuperItem = true;
        [[fallthrough]];
    case BST_ITEM:
        Items = Min<ui8> (Items + 1, (ui8) MaxItems);
        break;
    case BST_PARAGRAPH:
        Paragraphs = Min<ui8> (Paragraphs + 1, (ui8) MaxParagraphs);
        break;
    case BST_BLOCK:
        Blocks = Min<ui8> (Blocks + 1, (ui8) MaxBlocks);
        break;
    }
}

TString TBlockInfo::ToString() const {
    return Sprintf("{ HasSuperItem=%u, Items=%u, Paragraphs=%u, Blocks=%u, Included=%u }",
            HasSuperItem, Items, Paragraphs, Blocks, Included);
}

TBlockDist::TBlockDist(TBlockInfo a, TBlockInfo b)
    : SuperItems(a.HasSuperItem + b.HasSuperItem)
    , Items(a.Items + b.Items)
    , Paragraphs(a.Paragraphs + b.Paragraphs)
    , Blocks(a.Blocks + b.Blocks)
{
}

TBlockDist TBlockDist::Max() {
    TBlockDist max;
    max.Depth = 0xffffffff;
    return max;
}

TBlockDist TBlockDist::Make(ui32 blocks, ui32 pars, ui32 items, ui32 super) {
    TBlockDist i;
    i.Blocks = (ui8)blocks;
    i.Paragraphs = (ui8)pars;
    i.Items = (ui8)items;
    i.SuperItems = (ui8)super;
    return i;
}


TSegmentSpan::TSegmentSpan(TPosting begin, TPosting end)
    : TSpan(begin, end)
    , Type(0)
    , Number()
    , Total()
    , Weight(Min<float>())
{
    Zero(Bytes);
}

void TSegmentSpan::MergePrev(const TSegmentSpan& prev) {
    TSpan::MergePrev(prev);

    bool noblock = !TBlockDist(prev.LastBlock, FirstBlock).Depth;

    FirstSignature = prev.FirstSignature;
    FirstBlock = prev.FirstBlock;
    FirstTag = prev.FirstTag;

    MergeWith(prev);

    Number = prev.Number;
    AdsCSS = prev.AdsCSS;
    AdsHeader = prev.AdsHeader;
    FooterCSS = prev.FooterCSS;
    PollCSS = prev.PollCSS;
    MenuCSS = prev.MenuCSS;
    CommentsCSS = prev.CommentsCSS;
    CommentsHeader = prev.CommentsHeader;

    Blocks -= noblock;
}

void TSegmentSpan::MergeNext(const TSegmentSpan& next) {
    TSpan::MergeNext(next);

    bool noblock = !TBlockDist(next.FirstBlock, LastBlock).Depth;

    LastSignature = next.LastSignature;
    LastBlock = next.LastBlock;
    LastTag = next.LastTag;

    MergeWith(next);

    Blocks -= noblock;
}

TString TSegmentSpan::ToString(bool verbose) const {
    const char * format = verbose ?
                    "{ FirstSignature=%#x, LastSignature=%#x, \n"
                    "FirstBlock=%s, \n"
                    "LastBlock=%s, \n"
                    "Links=%u, LocalLinks=%u, FragmentLinks=%u, Domains=%u, Inputs=%u, Blocks=%u, \n"
                    "HeadersCount=%u, TitleWords=%u, FooterWords=%u, \n"
                    "Words=%u, LinkWords=%u, Symbols=%u, Commas=%u, Spaces=%u, Alphas=%u, \n"
                    "FirstTag=%u, LastTag=%u, FrontAbsDepth=%u, BackAbsDepth=%u, \n"
                    "Markers={ AdsHref=%u, AdsCSS=%u, AdsHeader=%u, FooterCSS=%u, FooterText=%u, \n"
                    "CommentsCSS=%u, CommentsHeader=%u, PollCSS=%u, MenuCSS=%u, IsHeader=%u, HasHeader=%u, \n"
                    "InArticle=%u, InMainContentReadability=%u, InMainContentNews=%u, HasMainHeaderNews=%u, HasSelfLink=%u, \n"
                    "MainContentFrontAdjoining=%u, MainContentBackAdjoining=%u, \n"
                    "MainContentWeight=%.3f }"
                    : "{ FirstSignature=%#x, LastSignature=%#x, "
                    "FirstBlock=%s, LastBlock=%s, "
                    "Links=%u, LocalLinks=%u, FragmentLinks=%u, Domains=%u, Inputs=%u, Blocks=%u, "
                    "HeadersCount=%u, TitleWords=%u, FooterWords=%u, "
                    "Words=%u, LinkWords=%u, Symbols=%u, Commas=%u, Spaces=%u, Alphas=%u, "
                    "FirstTag=%u, LastTag=%u, FrontAbsDepth=%u, BackAbsDepth=%u, "
                    "Markers=%u%u%u|%u%u|%u%u|%u%u|%u%u|%u%u%u%u|%u|%u%u, "
                    "Weight=%.3f }";
    return Sprintf(format,
                   FirstSignature, LastSignature,
                   FirstBlock.ToString().c_str(), LastBlock.ToString().c_str(),
                   Links, LocalLinks, FragmentLinks, Domains, Inputs, Blocks,
                   HeadersCount, TitleWords, FooterWords,
                   Words, LinkWords, SymbolsInText, CommasInText, SpacesInText, AlphasInText,
                   FirstTag, LastTag, FrontAbsDepth, BackAbsDepth,
                   AdsHref, AdsCSS, AdsHeader, FooterCSS, FooterText,
                   CommentsCSS, CommentsHeader, PollCSS, MenuCSS, IsHeader, HasHeader,
                   InArticle, InReadabilitySpans, InMainContentNews, HasMainHeaderNews, HasSelfLink,
                   MainContentFrontAdjoining, MainContentBackAdjoining, GetMainContentWeight());
}

void TSegmentSpan::MergeWith(const TSegmentSpan& seg) {
    FooterText |= seg.FooterText;
    IsHeader = false;
    HasHeader |= seg.HasHeader;
    AdsHref |= seg.AdsHref;
    InArticle |= seg.InArticle;
    InReadabilitySpans |= seg.InReadabilitySpans;
    InMainContentNews |= seg.InMainContentNews;
    HasMainHeaderNews |= seg.HasMainHeaderNews;
    MainContentFrontAdjoining |= seg.MainContentFrontAdjoining;
    MainContentBackAdjoining |= seg.MainContentBackAdjoining;
    MainWeight = f16::New(Max(GetMainContentWeight(), seg.GetMainContentWeight())).val;

    --Total;

    TitleWords = Max(TitleWords, seg.TitleWords);
    FooterWords = Max(FooterWords, seg.FooterWords);

    CheckedAdd(Words, seg.Words);
    CheckedAdd(LinkWords, seg.LinkWords);
    CheckedAdd(Links, seg.Links);
    CheckedAdd(LocalLinks, seg.LocalLinks);
    CheckedAdd(Domains, seg.Domains);
    CheckedAdd(Inputs, seg.Inputs);
    CheckedAdd(Blocks, seg.Blocks);
    CheckedAdd(HeadersCount, seg.HeadersCount);
    CheckedAdd(SymbolsInText, seg.SymbolsInText);
    CheckedAdd(SpacesInText, seg.SpacesInText);
    CheckedAdd(CommasInText, seg.CommasInText);
    CheckedAdd(AlphasInText, seg.AlphasInText);
}

TStoreSegmentSpanData CreateStoreSegmentSpanData(const TSegmentSpan& sp)
{
    TStoreSegmentSpanData data;
    data.FirstBlock = sp.FirstBlock;
    data.LastBlock = sp.LastBlock;

    data.Words = sp.Words;
    data.Domains = sp.Domains;
    data.LinkWords = sp.LinkWords;
    data.Links = sp.Links;
    data.LocalLinks = sp.LocalLinks;
    data.Inputs = sp.Inputs;
    data.Blocks = sp.Blocks;

    data.CommentsCSS = sp.CommentsCSS;
    data.AdsCSS = sp.AdsCSS;
    data.AdsHeader = sp.AdsHeader;
    data.FooterCSS = sp.FooterCSS;
    data.PollCSS = sp.PollCSS;
    data.MenuCSS = sp.MenuCSS;
    data.IsHeader = sp.IsHeader;
    data.HasHeader = sp.HasHeader;

    data.AllMarkers3 = sp.AllMarkers3;
    data.InMainContentNews = sp.InMainContentNews;
    data.HasMainHeaderNews = sp.HasMainHeaderNews;

    data.HeadersCount = sp.HeadersCount;
    data.TitleWords = sp.TitleWords;
    data.CommasInText = sp.CommasInText;
    data.SpacesInText = sp.SpacesInText;
    data.AlphasInText = sp.AlphasInText;
    data.SymbolsInText = sp.SymbolsInText;
    data.MainWeight = sp.MainWeight;
    return data;
}

void FillSegmentSpanFromStoreSegmentSpanData(const TStoreSegmentSpanData& data, TSegmentSpan& sp)
{
    sp.FirstBlock = data.FirstBlock;
    sp.LastBlock = data.LastBlock;

    sp.Words = data.Words;
    sp.LinkWords = data.LinkWords;
    sp.Links = data.Links;
    sp.Domains = data.Domains;
    sp.LocalLinks = data.LocalLinks;
    sp.Inputs = data.Inputs;
    sp.Blocks = data.Blocks;

    sp.CommentsCSS = data.CommentsCSS;
    sp.AdsCSS = data.AdsCSS;
    sp.AdsHeader = data.AdsHeader;
    sp.FooterCSS = data.FooterCSS;
    sp.PollCSS = data.PollCSS;
    sp.MenuCSS = data.MenuCSS;
    sp.IsHeader = data.IsHeader;
    sp.HasHeader = data.HasHeader;

    sp.AllMarkers3 = data.AllMarkers3;
    sp.InMainContentNews = data.InMainContentNews;
    sp.HasMainHeaderNews = data.HasMainHeaderNews;
    sp.HeadersCount = data.HeadersCount;
    sp.TitleWords = data.TitleWords;
    sp.CommasInText = data.CommasInText;
    sp.SpacesInText = data.SpacesInText;
    sp.AlphasInText = data.AlphasInText;
    sp.SymbolsInText = data.SymbolsInText;
    sp.MainWeight = data.MainWeight;
}

}
