#include "basesnip_impl.h"
#include "basesnip_viewers.h"
#include "hilite_messer.h"
#include "shuffle_words.h"
#include "pseudo_rand.h"

#include <kernel/snippets/archive/doc_lang.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/viewers.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/custom/clicklike.h>
#include <kernel/snippets/custom/creativework.h>
#include <kernel/snippets/custom/docsig.h>
#include <kernel/snippets/custom/dmoz.h>
#include <kernel/snippets/custom/replacer_chooser.h>
#include <kernel/snippets/custom/encyc.h>
#include <kernel/snippets/custom/entity.h>
#include <kernel/snippets/custom/entity_viewer.h>
#include <kernel/snippets/custom/extended.h>
#include <kernel/snippets/custom/listsnip.h>
#include <kernel/snippets/custom/list_handler.h>
#include <kernel/snippets/custom/market.h>
#include <kernel/snippets/custom/mediawiki.h>
#include <kernel/snippets/custom/news.h>
#include <kernel/snippets/custom/dicacademic.h>
#include <kernel/snippets/custom/statannot.h>
#include <kernel/snippets/custom/preview.h>
#include <kernel/snippets/custom/remove_emoji/remove_emoji.h>
#include <kernel/snippets/custom/canonize_unicode/canonize_unicode.h>
#include <kernel/snippets/custom/trash_annotation.h>
#include <kernel/snippets/custom/yaca.h>
#include <kernel/snippets/custom/kinopoisk.h>
#include <kernel/snippets/custom/tablesnip.h>
#include <kernel/snippets/custom/table_handler.h>
#include <kernel/snippets/custom/video_desc.h>
#include <kernel/snippets/custom/forums.h>
#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/custom/suppress_cyr.h>
#include <kernel/snippets/custom/movie.h>
#include <kernel/snippets/custom/rating.h>
#include <kernel/snippets/custom/opengraph/og.h>
#include <kernel/snippets/custom/sahibinden.h>
#include <kernel/snippets/custom/productoffer.h>
#include <kernel/snippets/custom/eksisozluk.h>
#include <kernel/snippets/custom/youtube_channel.h>
#include <kernel/snippets/custom/robots_txt.h>
#include <kernel/snippets/custom/question.h>
#include <kernel/snippets/custom/simple_meta.h>
#include <kernel/snippets/custom/software.h>
#include <kernel/snippets/custom/socnet.h>
#include <kernel/snippets/custom/generic_for_mobile.h>
#include <kernel/snippets/custom/hilitedurl.h>
#include <kernel/snippets/custom/fake_redirect.h>
#include <kernel/snippets/custom/need_translate.h>
#include <kernel/snippets/custom/static_extended.h>
#include <kernel/snippets/custom/fact_snip.h>
#include <kernel/snippets/custom/isnip.h>
#include <kernel/snippets/custom/robot_dater.h>
#include <kernel/snippets/custom/translated_doc.h>

#include <kernel/snippets/cut/cut.h>

#include <kernel/snippets/explain/cookie.h>
#include <kernel/snippets/explain/dump.h>
#include <kernel/snippets/explain/finaldump.h>
#include <kernel/snippets/explain/finaldumpbin.h>
#include <kernel/snippets/explain/texthits.h>
#include <kernel/snippets/explain/top_candidates.h>
#include <kernel/snippets/explain/losswords.h>

#include <kernel/snippets/hits/ctx.h>

#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereply.h>

#include <kernel/snippets/replace/replace.h>

#include <kernel/snippets/schemaorg/question/question.h>
#include <kernel/snippets/simple_textproc/decapital/decapital.h>

#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/enhance.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/sent_match.h>

#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/smartcut/smartcut.h>
#include <kernel/snippets/smartcut/snip_length.h>
#include <kernel/snippets/smartcut/hilited_length.h>

#include <kernel/snippets/snip_proto/snip_proto_2.h>

#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/hlmarks.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/titles_additions.h>
#include <kernel/snippets/titles/header_based.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/snippets/video/video.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <kernel/word_hyphenator/word_hyphenator.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/random/mersenne.h>
#include <library/cpp/string_utils/url/url.h>


namespace {

    void DecapSentAndDump(TUtf16String &w, ELanguage lang, TString message, NSnippets::ISnippetDebugOutputHandler* debug) {
        if (debug) {
            TUtf16String tmp = w;
            NSnippets::DecapitalSentence(w, lang);
            if (w != tmp) {
                 debug->Print(false, "%s. Origin value was \"%s\"", message.data(), WideToUTF8(tmp).data());
            }
        } else {
            NSnippets::DecapitalSentence(w, lang);
        }
    }

    void CutStringIfNeed(const size_t allowSymbolsCount, size_t& usedSymbolsCount, TUtf16String& snippet) {
        if (usedSymbolsCount >= allowSymbolsCount) {
            snippet.clear();
        } else if (usedSymbolsCount + snippet.length() <= allowSymbolsCount) {
            usedSymbolsCount += snippet.length();
        } else {
            snippet = NSnippets::SmartCutSymbol(snippet, allowSymbolsCount - usedSymbolsCount);
            usedSymbolsCount = allowSymbolsCount;
        }
    }

} //namespace

namespace NSnippets {

static const TString HOSTNAMES_FOR_DEFINITIONS = "hostnames_for_definitions";

class TFillPassageReplyImpl : public TFillPassageReply::IImpl {
private:
    TPassageReply& Reply;
    const TConfig& Config;
    TLengthChooser LengthChooser;
    TArchiveStorage Storage;
    TArchiveMarkup Markup;
    const THitsInfoPtr HitsInfo;
    const TInlineHighlighter& IH;
    TArcManip& ArcCtx;
    const TQueryy& QueryCtx;
    NUrlCutter::TRichTreeWanderer& RichtreeWanderer;
    const TDocInfos& DocInfos;
    const ELanguage DocLangId;
    const TString Url;
    THolder<ISnippetsCallback> ExplainCallback;
    THolder<TTopCandidateCallback> FactSnippetTopCandidatesCallback;
    THolder<TWordSpanLen> SnipWordSpanLen;

    bool IsByLink;
    THolder<const TSentsInfo> SentsInfo;
    THolder<const TSentsMatchInfo> SentsMatchInfo;
    THolder<TMetaDescription> MetaDescr;
    THolder<TSnipTitleSupplementer> TitleSupplementer;
    TSnipTitle NaturalTitle;
    TUtf16String NaturalTitleSource;
    TSnipTitle SuperNaturalTitle;
    TSnip Snip;
    TSnip OneFragmentSnip;
    TExtraSnipAttrs ExtraSnippetAttrs;
    THolder<TReplaceManager> ReplaceManager;

private:
    const TReplaceContext* CreateReplaceContext()
    {
        return new TReplaceContext(
            QueryCtx,
            *SentsMatchInfo,
            Snip,
            NaturalTitle,
            NaturalTitleSource,
            SuperNaturalTitle,
            ArcCtx.GetTextArc().GetDescr().get_encoding(),
            DocInfos,
            Url,
            *MetaDescr.Get(),
            IH,
            RichtreeWanderer,
            IsByLink,
            DocLangId,
            HitsInfo.Get() && HitsInfo->IsNav,
            Config,
            LengthChooser,
            Markup,
            *SnipWordSpanLen,
            OneFragmentSnip);
    }

    void PrepareSnipInfo(const TSentHandler& shWrapper) {
        Config.LogPerformance("GetSnip.Start");
        TArchiveView metaDescrAdd;
        if (!IsByLink && MetaDescr.Get() && MetaDescr->MayUse()
            && !shWrapper.GetForumsViewer().FilterByMessages())
        {
            metaDescrAdd.PushBack(&*Storage.Add(ARC_MISC, 0, MetaDescr->GetTextCopy(), 0));
        }

        const TArchiveView& view = IsByLink ? shWrapper.GetLinkView() : shWrapper.GetTextView();
        SentsInfo.Reset(new TSentsInfo(&Markup, view, &metaDescrAdd, true, false));
        Config.LogPerformance("GetSnip.SentsInfo");
        SentsMatchInfo.Reset(new TSentsMatchInfo(*SentsInfo, QueryCtx, Config, DocLangId, SuperNaturalTitle.GetSentsMatchInfo()));
        Config.LogPerformance("GetSnip.SentsMatchInfo");
    }

    TSnip GetSnip(const TSentHandler& shWrapper)
    {
        Y_ASSERT(SentsInfo && SentsMatchInfo);
        float maxLenFactor = 1.0f;
        bool dontGrow = false;
        if (!IsByLink && shWrapper.GetForumsViewer().FilterByMessages() && Config.GetSnipCutMethod() == TCM_PIXEL) {
            maxLenFactor = 0.75f;
            dontGrow = true;
        }

        TSnip resSnip;
        if (!Config.IsUseStaticDataOnly()) {
            NSnipRedump::GetSnippet(Config, *SnipWordSpanLen,
                IsByLink ? nullptr : SentsMatchInfo.Get(),
                IsByLink ? SentsMatchInfo.Get() : nullptr,
                Config.GetRedump(), ExplainCallback->GetCandidateHandler(), "redump",
                !Config.TitWeight() ? nullptr : &SuperNaturalTitle,
                LengthChooser.GetMaxSnipLen(), Url, ReplaceManager->GetCustomSnippets());

            resSnip = GetBestSnip(*SentsMatchInfo, SuperNaturalTitle, Config, Url, LengthChooser,
                *SnipWordSpanLen, *ExplainCallback, !!FactSnippetTopCandidatesCallback ? FactSnippetTopCandidatesCallback.Get() : nullptr , IsByLink, maxLenFactor, dontGrow, &OneFragmentSnip,
                &shWrapper.GetSchemaOrgViewer());
            if (Config.EnableRemoveDuplicates()) {
                bool removed = resSnip.RemoveDuplicateSnips();
                if (removed && ExplainCallback->GetDebugOutput()) {
                    ExplainCallback->GetDebugOutput()->Print(false, "Duplicate snippet fragment was removed");
                }
            }
            AppendRandomWords(resSnip, Config.GetAppendRandomWordsCount());
            Config.LogPerformance("GetSnip.BestSnip");
        }

        Config.LogPerformance("GetSnip.Stop");
        return resSnip;
    }

    // SNIPPETS-7243
    void AppendRandomWords(TSnip& snip, ui32 wordsCount) {
        if (!wordsCount || snip.Snips.empty())
            return;
        TSingleSnip& lastFragment = snip.Snips.back();
        const TSentsMatchInfo* sents = lastFragment.GetSentsMatchInfo();
        Y_ASSERT(sents);
        int chooseBegin = lastFragment.GetLastSent() + 1;
        int chooseEnd = sents->SentsInfo.SentencesCount();
        if (chooseBegin >= chooseEnd)
            return;
        ui32 maxWords = 0;
        TVector<int> candidates;
        for (int i = chooseBegin; i < chooseEnd; i++) {
            ui32 curWords = sents->SentsInfo.GetSentLengthInWords(i);
            maxWords = Max(maxWords, curWords);
            if (curWords >= wordsCount)
                candidates.push_back(i);
        }
        if (!maxWords)
            return;
        if (candidates.empty()) {
            for (int i = chooseBegin; i < chooseEnd; i++)
                if ((ui32)sents->SentsInfo.GetSentLengthInWords(i) == maxWords)
                    candidates.push_back(i);
        }
        Y_ASSERT(!candidates.empty());
        size_t pseudoRand = PseudoRand(Url, HitsInfo);
        TMersenne<ui32> mersenn(pseudoRand);
        int chosen = candidates[mersenn.Uniform(candidates.size())];
        int usedWordsCount = Min<int>(sents->SentsInfo.GetSentLengthInWords(chosen), wordsCount);
        Y_ASSERT(usedWordsCount);
        if (sents->SentsInfo.GetOrigSentId(chosen) == sents->SentsInfo.GetOrigSentId(chooseBegin - 1) + 1 && !lastFragment.EndsWithSentBreak()) {
            lastFragment.SetWordRange(lastFragment.GetFirstWord(), lastFragment.GetLastWord() + usedWordsCount);
        } else {
            int firstWord = sents->SentsInfo.FirstWordIdInSent(chosen);
            snip.Snips.push_back(TSingleSnip(firstWord, firstWord + usedWordsCount - 1, *sents));
        }
    }

    bool ReplaceByFakeDocTitle(TUtf16String& title) {
        ArcCtx.GetLinkArc().FillDescr();
        if (ArcCtx.GetLinkArc().IsValid()) {
            const TDocInfos::iterator it = ArcCtx.GetLinkArc().GetDocInfosPtr()->find("fake_doc_title");
            if (it != ArcCtx.GetLinkArc().GetDocInfosPtr()->end()) {
                title = UTF8ToWide(it->second);
                return true;
            }
        }
        return false;
    }

    void GenerateTitle(TSentHandler& shWrapper) {
        TVector<TUtf16String> vTitles;
        DumpResultCopy(shWrapper.GetMetadataViewer().Title, vTitles);
        TUtf16String tmpTitle = GlueTitle(vTitles);
        const size_t TITLE_RUDE_CUT = 1000;
        tmpTitle.remove(TITLE_RUDE_CUT);
        if (Config.NeedDecapital()) {
            DecapSentAndDump(tmpTitle, DocLangId,
                "Title was decapitalized during reading from t-Archive.", ExplainCallback->GetDebugOutput());
        }
        Strip(tmpTitle);
        ClearChars(tmpTitle, /* allowSlash */ false, Config.AllowBreveInTitle());

        bool questionTitle = Config.QuestionTitles() &&
                             GetOnlyHost(Url) == "otvet.mail.ru";
        if (questionTitle && Config.QuestionTitlesApproved()) {
           const NSchemaOrg::TQuestion* question = shWrapper.GetSchemaOrgViewer().GetQuestion();
           if (question == nullptr || !question->HasApprovedAnswer()) {
               questionTitle = false;
           }
        }
        const bool hostnameForDefinition = (Config.IsPadReport() ?
                                            Config.ExpFlagOn(HOSTNAMES_FOR_DEFINITIONS) :
                                            !Config.ExpFlagOff(HOSTNAMES_FOR_DEFINITIONS));

        TMakeTitleOptions options(Config);
        if (hostnameForDefinition || questionTitle) {
            options.Url = Url;
            options.HostnamesForDefinitions = true;
        }
        NaturalTitle = MakeTitle(tmpTitle, Config, QueryCtx, options);
        NaturalTitleSource = tmpTitle;

        options.Url.clear();
        options.HostnamesForDefinitions = false;

        TString urlmenu;
        auto arcIt = DocInfos.find("urlmenu");
        if (arcIt != DocInfos.end()) {
            urlmenu = arcIt->second;
        }
        TitleSupplementer.Reset(new TSnipTitleSupplementer(Config, QueryCtx, TMakeTitleOptions(Config), DocLangId, Url, urlmenu, NaturalTitle.GetDefinition(), questionTitle, *ExplainCallback));

        if (ExplainCallback->GetCandidateHandler()) {
            ExplainCallback->GetCandidateHandler()->AddTitleCandidate(NaturalTitle, TS_NATURAL);
        }

        if (Config.ForcePureTitle()) {
            SuperNaturalTitle = NaturalTitle;
            return;
        }

        if (hostnameForDefinition) {
            TitleSupplementer->EliminateDefinitionFromTitle(NaturalTitle);
        }
        TitleSupplementer->GenerateTwoLineTitle(NaturalTitle);

        if (Config.VideoTitles()) {
            if (GenerateVideoTitle(SuperNaturalTitle, shWrapper.GetTextView(), QueryCtx, options, DocInfos, Config, NaturalTitle)) {
                if (ExplainCallback->GetDebugOutput()) {
                    ExplainCallback->GetDebugOutput()->Print(false, "Alternative video title is chosen: %s", WideToUTF8(SuperNaturalTitle.GetTitleString()).data());
                }
                return;
            }
        }

        if (GenerateHeaderBasedTitle(SuperNaturalTitle, shWrapper.GetHeaderViewer(), QueryCtx, NaturalTitle, options, Config, Url, ExplainCallback->GetCandidateHandler())) {
            if (ExplainCallback->GetDebugOutput()) {
                ExplainCallback->GetDebugOutput()->Print(false, "Alternative title is chosen: %s", WideToUTF8(SuperNaturalTitle.GetTitleString()).data());
            }
            return;
        }

        if (Config.OpenGraphInTitles()) {
            TitleSupplementer->GenerateOpenGraphBasedTitle(NaturalTitle, DocInfos);
        }
        if (Config.MetaDescriptionsInTitles()) {
            TitleSupplementer->GenerateMetaDescriptionBasedTitle(NaturalTitle, *MetaDescr.Get());
        }

        if (tmpTitle.empty()) {
            ReplaceByFakeDocTitle(tmpTitle);
            ClearChars(tmpTitle, false, Config.AllowBreveInTitle());
            SuperNaturalTitle = MakeTitle(tmpTitle, Config, QueryCtx, options);
        } else {
            SuperNaturalTitle = NaturalTitle;
        }
    }

    void Hyphenate(TUtf16String& line, const THyphenateParams& params) {
        line = TWordHyphenator().Hyphenate(line, params);
    }

    TVector<TUtf16String> RemoveEmojiAndNormalizeUnicode(const TVector<TUtf16String>& passages) {
        TVector<TUtf16String> result;
        size_t totalEmoji = 0;
        for (const auto& passage : passages) {
            totalEmoji += CountEmojis(passage);
        }
        result.reserve(passages.size());
        for (auto passage : passages) {
            if (NeedRemoveEmojis(Config, QueryCtx, Url) && totalEmoji > Config.AllowedExtSnippetEmojiCount()) {
                RemoveEmojis(passage);
            }
            if (Config.ShouldCanonizeUnicode()) {
                CanonizeUnicode(passage);
            }
            result.push_back(passage);
        }
        return result;
    }

    TVector<TUtf16String> AddExtSnippet(const TPaintingOptions& paintingOptions,
        bool addHyphenation, const THyphenateParams& hyphenateParams)
    {
        TVector<TUtf16String> result;
        TVector<TZonedString> extendedSnipZoneVec;
        if (!ExtraSnippetAttrs.GetExtendedSnippet().Snips.empty()) {
            extendedSnipZoneVec = ExtraSnippetAttrs.GetExtendedSnippet().GlueToZonedVec(false, TSnip());
            TEnhanceSnippetConfig cfg{Config, IsByLink, DocLangId, ExplainCallback->GetDebugOutput(), Url};
            EnhanceSnippet(cfg, extendedSnipZoneVec, ExtraSnippetAttrs);
            IH.PaintPassages(extendedSnipZoneVec, paintingOptions);
            for (TUtf16String& passage : RemoveEmojiAndNormalizeUnicode(MergedGlue(extendedSnipZoneVec))) {
                if (addHyphenation) {
                    Hyphenate(passage, hyphenateParams);
                }
                result.push_back(passage);
            }
        } else if (TUtf16String headlineStr = ExtraSnippetAttrs.GetExtendedHeadline().Long) {
            FixWeirdChars(headlineStr);
            extendedSnipZoneVec.emplace_back(TZonedString(headlineStr));
            IH.PaintPassages(extendedSnipZoneVec.back(), paintingOptions);
            headlineStr = MergedGlue(extendedSnipZoneVec.back());
            if (addHyphenation) {
                Hyphenate(headlineStr, hyphenateParams);
            }
            result.push_back(headlineStr);
        }

        if (!result.empty()) {
            NJson::TJsonValue passages(NJson::JSON_ARRAY);
            for (const auto& passage : result) {
                passages.AppendValue(WideToUTF8(passage));
            }
            NJson::TJsonValue features(NJson::JSON_MAP);
            features["passages"] = passages;
            features["type"] = "extended_snippet";
            NJson::TJsonValue data(NJson::JSON_MAP);
            data["block_type"] = "construct";
            data["content_plugin"] = true;
            data["features"] = features;
            data["slot"] = "pre";

            ExtraSnippetAttrs.AddClickLikeSnipJson("extended_snippet", NJson::WriteJson(data));
        }
        return result;
    }

    void AddExtHeadlineMarkers(TZonedString& zonedHeadline, int prevHeadlinePrefixLen) {
        TWtringBuf bounds(zonedHeadline.String.data(), zonedHeadline.String.data() + prevHeadlinePrefixLen);
        zonedHeadline.GetOrCreateZone(+TZonedString::ZONE_EXTSNIP, &EXT_MARK).Spans.emplace_back(bounds);
    }

    void Finalize()
    {
        if (ExplainCallback->GetDebugOutput() && HitsInfo.Get()) {
            ExplainCallback->GetDebugOutput()->Print(false, "%s", HitsInfo->DebugString().data());
        }
        const bool addExtendedConstruct = (!Config.IsNeedExt() && !Config.ExpFlagOff("add_extended")) || Config.ExpFlagOn("add_extended");
        TSnip prevSnip;
        if (!addExtendedConstruct && Config.IsNeedExt() && ExtraSnippetAttrs.GetExtendedSnippet().Snips) {
            if (Config.NeedExtDiff()) {
                prevSnip = Snip;
            }
            Snip = ExtraSnippetAttrs.GetExtendedSnippet();
        }
        TVector<TZonedString> snipVec = Snip.GlueToZonedVec(false, prevSnip);
        TEnhanceSnippetConfig cfg{Config, IsByLink, DocLangId, ExplainCallback->GetDebugOutput(), Url};
        EnhanceSnippet(cfg, snipVec, ExtraSnippetAttrs);

        if (!ReplaceManager->IsReplaced()) {
            Snip.GuessAttrs();
        }
        TVector<TString> attrVec = Snip.DumpAttrs();

        if (Config.VideoHide()) {
            HideVideoSnippets(snipVec, attrVec, Config, LengthChooser, Storage, ArcCtx.GetTextArc());
        }

        TUtf16String headline;
        int prevHeadlinePrefixLen = 0;
        TString headlineSrc;
        TUtf16String title = SuperNaturalTitle.GetTitleString();

        const TReplaceResult& replaceResult = ReplaceManager->GetResult();
        if (replaceResult.CanUse()) {
            headline = replaceResult.GetText();
            const auto& extHeadline = ExtraSnippetAttrs.GetExtendedHeadline();
            if (!addExtendedConstruct && Config.IsNeedExt() && extHeadline.Long) {
                headline = extHeadline.Long;
                prevHeadlinePrefixLen = extHeadline.CommonPrefixLen;
            }
            headlineSrc = replaceResult.GetTextSrc();
        }

        if (Config.NeedDecapital()) {
            DecapSentAndDump(title, DocLangId, "Final title was decapitalized.", ExplainCallback->GetDebugOutput());
        }

        FixWeirdChars(title);
        FixWeirdChars(headline);

        if (ExplainCallback->GetDebugOutput()) {
            ExplainCallback->GetDebugOutput()->Print(false, "Final title: \"%s\"", WideToUTF8(title).data());
            ExplainCallback->GetDebugOutput()->Print(false, "Final headline: \"%s\"", WideToUTF8(headline).data());
            for (ui16 i = 0; i < snipVec.size(); ++i)
                ExplainCallback->GetDebugOutput()->Print(false, "Final passage[%u]: \"%s\"", i, WideToUTF8(snipVec[i].String).data());
        }

        ExplainCallback->OnTitleReplace(ReplaceManager->GetResult().GetTitle());

        TPaintingOptions paintingOptions = TPaintingOptions::DefaultSnippetOptions();
        paintingOptions.SkipAttrs = Config.PaintNoAttrs();
        paintingOptions.Fred = Config.Fred();
        if (!Config.UnpaintTitle()) {
            IH.PaintPassages(title, paintingOptions);
        }
        TZonedString zonedHeadline = headline;

        IH.PaintPassages(zonedHeadline, paintingOptions);
        IH.PaintPassages(snipVec, paintingOptions);

        TVector<TZonedString> paintedFragments;
        if (zonedHeadline.String) {
            paintedFragments.push_back(zonedHeadline);
        }
        for (const TZonedString& passage : snipVec) {
            paintedFragments.push_back(passage);
        }

        const bool addHyphenation = Config.ExpFlagOn("add_hyphenation");
        const size_t minSymbolsBeforeHyphenation = Config.GetMinSymbolsBeforeHyphenation();
        const size_t minWordLengthForHyphenation = Config.GetMinWordLengthForHyphenation();
        const auto hyphenateParams = THyphenateParams(minSymbolsBeforeHyphenation, minWordLengthForHyphenation);
        bool willFillReadMoreLine = false;
        TVector<TUtf16String> extendedSnipVec;
        if (addExtendedConstruct) {
            extendedSnipVec = AddExtSnippet(paintingOptions, addHyphenation, hyphenateParams);
            willFillReadMoreLine = !extendedSnipVec.empty()  && !Config.ExpFlagOff("fill_read_more_line") && Config.GetSnipCutMethod() == TCM_PIXEL;
        }
        if (prevHeadlinePrefixLen && Config.NeedExtDiff()) {
            AddExtHeadlineMarkers(zonedHeadline, prevHeadlinePrefixLen);
        }

        TPassageReplyData replyData;
        replyData.LinkSnippet = IsByLink;
        replyData.Passages = MergedGlue(snipVec);
        replyData.Attrs = attrVec;
        replyData.Title = title;
        replyData.Headline = MergedGlue(zonedHeadline);
        if (prevHeadlinePrefixLen) {
            replyData.Passages = {replyData.Headline};
            replyData.Headline.clear();
        }
        replyData.HeadlineSrc = headlineSrc;
        replyData.SnipLengthInSymbols = GetHilitedTextLengthInSymbols(paintedFragments);
        replyData.SnipLengthInRows = GetHilitedTextLengthInRows(paintedFragments, Config.GetYandexWidth(), Config.GetSnipFontSize());
        if (willFillReadMoreLine) {
            FillReadMoreLineInReply(replyData, paintedFragments, extendedSnipVec, IH, paintingOptions, Config, ExplainCallback.Get());
        }
        if (addHyphenation) {
            Hyphenate(replyData.Headline, hyphenateParams);
            for (TUtf16String& passage : replyData.Passages) {
                Hyphenate(passage, hyphenateParams);
            }
        }

        if (const size_t allowSymbolsCount = Config.ForceCutSnip()) {
            size_t usedSymbolsCount = 0;
            for (auto& passage : replyData.Passages) {
                CutStringIfNeed(allowSymbolsCount, usedSymbolsCount, passage);
            }
            CutStringIfNeed(allowSymbolsCount, usedSymbolsCount, replyData.Headline);
        }
        replyData.SpecSnippetAttrs = ExtraSnippetAttrs.GetSpecAttrs();
        replyData.LinkAttrs = ExtraSnippetAttrs.GetLinkAttrs();
        replyData.DocQuerySig = ExtraSnippetAttrs.GetDocQuerySig();
        replyData.DocStaticSig = ExtraSnippetAttrs.GetDocStaticSig();
        replyData.EntityClassifyResult = ExtraSnippetAttrs.GetEntityClassifyResult();
        replyData.ClickLikeSnip = ExtraSnippetAttrs.GetPackedClickLikeSnip();
        replyData.PreviewJson = ExtraSnippetAttrs.GetPreviewJson();
        replyData.RawPreview = ExtraSnippetAttrs.GetRawPreview();
        replyData.SchemaVthumb = ExtraSnippetAttrs.GetSchemaVthumb();
        replyData.UrlMenu = ExtraSnippetAttrs.GetUrlmenu();
        replyData.HilitedUrl = ExtraSnippetAttrs.GetHilitedUrl();
        Reply.Set(replyData);

        if (Config.ShareMessedSnip()) { // in messy experiment
            MessWithReply(Config, QueryCtx, Url, HitsInfo, Reply);
        }
        if (Config.ShuffleWords()) {
            ShuffleWords(Config, Url, HitsInfo, Reply);
        }

        Reply.AddMarker(ExtraSnippetAttrs.Markers.Dump());
        ExplainCallback->OnMarkers(ExtraSnippetAttrs.Markers);

        cfg.OutputHandler = nullptr;
        ExplainCallback->OnPassageReply(Reply, cfg);
        {
            ExplainCallback->OnDocInfos(DocInfos);
            TBufferOutput explanation(0);
            ExplainCallback->GetExplanation(explanation);
            Reply.SetExplanation(TString(explanation.Buffer().Data(), explanation.Buffer().Size()));
        }

        Config.LogPerformance("FillReply.Finalize");
    }

    static TAutoPtr<ISnippetsCallback> CreateCallback(const TArchiveMarkup& markup, const TConfig& cfg, const TString& url, TExtraSnipAttrs& extraSnipAttrs, const TInlineHighlighter& ih)
    {
        if (cfg.GetInfoRequestType() == INFO_SNIPPETS) {
            return new TCookieCallback(cfg.GetInfoRequestParams());
        } else if (cfg.GetInfoRequestType() == INFO_SNIPPET_HITS) {
            return new TSnippetHitsCallback(cfg.GetInfoRequestParams());
        } else if (cfg.IsDumpCandidates()) {
            return new TDumpCallback(cfg);
        } else if (cfg.LossWords()) {
            return new TLossWordsCallback(markup, cfg, url);
        } else if (cfg.IsDumpFinalFactors()) {
            return new TFinalFactorsDump();
        } else if (cfg.IsDumpFinalFactorsBinary()) {
            return new TFinalFactorsDumpBinary(cfg.FactorsToDump());
        } else if (cfg.TopCandidatesRequested()) {
            return new TTopCandidateCallback(cfg, extraSnipAttrs, ih, true);
        } else {
            return new TSnippetsCallbackStub();
        }
    }

public:
    TFillPassageReplyImpl(TPassageReply& reply,
        const TConfig& cfg,
        const THitsInfoPtr hitsInfo,
        const TInlineHighlighter& ih,
        TArcManip& arcctx,
        const TQueryy& queryCtx,
        NUrlCutter::TRichTreeWanderer& richtreeWanderer
        )
        : Reply(reply)
        , Config(cfg)
        , LengthChooser(cfg, queryCtx)
        , Storage()
        , Markup()
        , HitsInfo(hitsInfo)
        , IH(ih)
        , ArcCtx(arcctx)
        , QueryCtx(queryCtx)
        , RichtreeWanderer(richtreeWanderer)
        , DocInfos(*ArcCtx.GetTextArc().GetDocInfosPtr())
        , DocLangId(GetDocLanguage(DocInfos, Config))
        , Url(ArcCtx.GetTextArc().GetDescr().get_url())
        , ExplainCallback(CreateCallback(Markup, Config, Url, ExtraSnippetAttrs, IH))
    {
        if (Config.GetSnipCutMethod() == TCM_SYMBOL) {
            SnipWordSpanLen.Reset(new TWordSpanLen(TCutParams::Symbol()));
        } else {
            SnipWordSpanLen.Reset(new TWordSpanLen(TCutParams::Pixel(Config.GetYandexWidth(), Config.GetSnipFontSize())));
        }
        if (Config.ExpFlagOn("fact_snip_candidates")) {
            FactSnippetTopCandidatesCallback.Reset(new TTopCandidateCallback(Config, ExtraSnippetAttrs, IH, false));
        }
        Markup.SetHitsInfo(HitsInfo);
        IsByLink = IsByLinkHitsPtr(HitsInfo);
    }

    void FillReply() override {
        Config.LogPerformance("FillReply.Start");

        TSentHandler shWrapper(Config, Storage, Markup, &QueryCtx, ArcCtx, ExplainCallback.Get(), Url, DocLangId);
        Config.LogPerformance("FillReply.InitSentWrappers");

        if (!shWrapper.DoUnpack(true, IsByLink)) { // failed
            Reply.SetError();
            Config.LogPerformance("FillReply.ForcedFinish");
            Config.LogPerformance("FillReply.Stop");
            return;
        }
        Config.LogPerformance("FillReply.UnpackSents");

        if (Config.UseTouchSnippetBody() && Config.ExpFlagOn("ratings_short_touch")) {
            if (TSchemaOrgRatingReplacer::HasRating(Config, shWrapper.GetSchemaOrgViewer())) {
                LengthChooser.SetShortenRowScale();
            }
        }

        MetaDescr.Reset(new TMetaDescription(Config, DocInfos, Url, DocLangId, HitsInfo.Get() && HitsInfo->IsNav));
        Config.LogPerformance("FillReply.MetaDescription");

        GenerateTitle(shWrapper);
        ExplainCallback->OnTitleSnip(NaturalTitle.GetTitleSnip(), SuperNaturalTitle.GetTitleSnip(), IsByLink);
        Config.LogPerformance("FillReply.GenerateTitle");

        PrepareSnipInfo(shWrapper);
        if (ExplainCallback->GetTextHandler(IsByLink)) {
            ExplainCallback->GetTextHandler(IsByLink)->AddHits(*SentsMatchInfo);
        }
        ReplaceManager.Reset(new TReplaceManager(CreateReplaceContext(), ExtraSnippetAttrs, *ExplainCallback, !!FactSnippetTopCandidatesCallback? FactSnippetTopCandidatesCallback.Get() : nullptr));
        Snip = GetSnip(shWrapper);
        if (ExplainCallback->GetTextHandler(IsByLink)) {
            ExplainCallback->GetTextHandler(IsByLink)->MarkFinalSnippet(Snip);
        }
        if (ExplainCallback->GetDebugOutput() && (!Config.UseTurkey() || Config.TrashPessTr())) {
            double shareOfTrash = ExplainCallback->GetShareOfTrashCandidates(IsByLink);
            ExplainCallback->GetDebugOutput()->Print(false, "Share of trash candidates = %.6lf", shareOfTrash);
        }

        Config.LogPerformance("FillReply.Replacers.Start");

        ReplaceManager->AddReplacer(new TSahibindenTemplatesReplacer());
        // dirty hack
        // maybe in future will be replaced by real infrastructure
        // for banned by robots.txt fake documents
        // SNIPPETS-2239
        ReplaceManager->AddReplacer(new TSahibindenFakeReplacer);

        if (Config.IsUseStaticDataOnly() && Config.IsMainContentStaticDataSource()) {
            ReplaceManager->AddReplacer(new TStaticAnnotationReplacer(shWrapper.GetStatAnnotViewer()));
        } else {
            ReplaceManager->AddReplacer(new TForumReplacer(shWrapper.GetForumsViewer()));
            if (Config.WeightedReplacers()) {
                ReplaceManager->AddReplacer(new TWeightedIdealCatalogReplacer);
                ReplaceManager->AddReplacer(new TWeightedYaCatalogReplacer);
                ReplaceManager->AddReplacer(new TWeightedVideoDescrReplacer);
            }
            ReplaceManager->AddReplacer(new TNewsReplacer);
            if (Config.YacaSnippetReplacerAllowed() && !Config.WeightedReplacers()) {
                ReplaceManager->AddReplacer(new TYacaReplacer);
            }
            if (Config.DmozSnippetReplacerAllowed() && !Config.WeightedReplacers()) {
                ReplaceManager->AddReplacer(new TDmozReplacer);
            }
            ReplaceManager->AddReplacer(new TEksisozlukReplacer(shWrapper.GetSchemaOrgViewer()));
            ReplaceManager->AddReplacer(new TQuestionReplacer(shWrapper.GetSchemaOrgViewer()));
            ReplaceManager->AddReplacer(new TEncycReplacer);
            if (Config.UseMediawiki()) {
                ReplaceManager->AddReplacer(new TMediaWikiReplacer);
            }
            ReplaceManager->AddReplacer(new TMovieReplacer(shWrapper.GetSchemaOrgViewer()));
            ReplaceManager->AddReplacer(new TKinoPoiskReplacer);
            if (Config.IsSlovariSearch()) {
                ReplaceManager->AddReplacer(new TDicAcademicReplacer);
            }
            ReplaceManager->AddReplacer(new TMarketReplacer);
            ReplaceManager->AddReplacer(new TYoutubeChannelReplacer(shWrapper.GetSchemaOrgViewer()));
            ReplaceManager->AddReplacer(new TSoftwareApplicationReplacer(shWrapper.GetSchemaOrgViewer()));
            ReplaceManager->AddReplacer(new TOgTextReplacer);
            ReplaceManager->AddReplacer(new TProductOfferReplacer(shWrapper.GetSchemaOrgViewer()));
            if (Config.WeightedReplacers()) {
                ReplaceManager->AddReplacer(new TWeightedMetaDescrReplacer);
            }
            ReplaceManager->AddReplacer(new TVideoReplacer);
            ReplaceManager->AddReplacer(new TFakeRedirectReplacer(HitsInfo.Get() && HitsInfo->IsFakeForRedirect));

            // always must be last replacers
            ReplaceManager->AddReplacer(new TStaticAnnotationReplacer(shWrapper.GetStatAnnotViewer()));
            ReplaceManager->AddReplacer(new TTrashAnnotationReplacer(shWrapper.GetTrashViewer()));
            if (Config.UseRobotsTxtStub()) {
                bool isFakeForBan = HitsInfo.Get() && HitsInfo->IsFakeForBan;
                ReplaceManager->AddReplacer(new TRobotsTxtStubReplacer(isFakeForBan));
            }
            if (Config.UseTableSnip() && !shWrapper.GetForumsViewer().IsValid()) {
                ReplaceManager->AddReplacer(new TTableSnipReplacer(shWrapper.GetTableViewer()));
            }
            if (Config.UseListSnip() && !shWrapper.GetForumsViewer().IsValid()) {
                ReplaceManager->AddReplacer(new TSnipListReplacer(shWrapper.GetListViewer()));
            }
            ReplaceManager->AddReplacer(new TMetaDescrReplacer);
        }

        if (Config.SuppressCyr() || Config.SuppressCyrForTr() || Config.SuppressCyrForSpok() && HitsInfo.Get() && HitsInfo->IsSpok) {
            ReplaceManager->AddPostReplacer(new TCyrillicToEmptyReplacer);
        }
        ReplaceManager->AddPostReplacer(new TClickLikeSnipDataReplacer);
        ReplaceManager->AddPostReplacer(new TSocnetDataReplacer);
        if (Config.AddGenericSnipForMobile()) {
            ReplaceManager->AddPostReplacer(new TGenericForMobileDataReplacer);
        }
        ReplaceManager->AddPostReplacer(new TISnipReplacer);
        ReplaceManager->AddPostReplacer(new TStaticExtendedReplacer(shWrapper.GetStatAnnotViewer()));
        ReplaceManager->AddPostReplacer(new TExtendedSnippetDataReplacer(shWrapper.GetExtendedTextView()));
        // Next replacer may remove parts of the snippet
        bool needRemoveEmojis = NeedRemoveEmojis(Config, QueryCtx, Url);
        if (needRemoveEmojis) {
            ReplaceManager->AddPostReplacer(new TRemoveEmojiReplacer);
        }
        ReplaceManager->AddPostReplacer(new TCanonizeUnicodeReplacer);
        // Past this point replacers shouldn't modify the snippet, neither the passages nor the body
        if (!Config.NoPreview()) {
            ReplaceManager->AddPostReplacer(new TPreviewReplacer(shWrapper.GetContentPreviewViewer(), shWrapper.GetForumsViewer()));
        }
        if (Config.HasEntityClassRequest()) {
            ReplaceManager->AddPostReplacer(new TEntityDataReplacer(shWrapper.GetEntityViewer().GetResult()));
        }
        ReplaceManager->AddPostReplacer(new THilitedUrlDataReplacer);
        ReplaceManager->AddPostReplacer(new TDocSigDataReplacer);
        ReplaceManager->AddPostReplacer(new TSchemaOrgRatingReplacer(shWrapper.GetSchemaOrgViewer()));
        ReplaceManager->AddPostReplacer(new TNeedTranslateReplacer());
        ReplaceManager->AddPostReplacer(new TFactSnipReplacer);
        ReplaceManager->AddPostReplacer(new TRobotDateReplacer());
        ReplaceManager->AddPostReplacer(new TTranslatedDocReplacer());

        ReplaceManager->DoWork();

        const TReplaceResult& res = ReplaceManager->GetResult();
        if (res.GetSnip()) {
            Snip = *res.GetSnip();
        }
        if (res.GetTitle()) {
            SuperNaturalTitle = *res.GetTitle();
        }

        if (!Config.ForcePureTitle() && TitleSupplementer.Get() != nullptr) {
            TitleSupplementer->GenerateUrlBasedTitle(SuperNaturalTitle, SentsInfo.Get());

            if (!Config.BNASnippets() && !Config.ShortYacaTitles() && !Config.EliminateDefinitions()) {
                if (Config.TitlesWithSplittedHostnames()) {
                    TitleSupplementer->AddSplittedHostNameToTitle(SuperNaturalTitle, SentsInfo.Get());
                }
                if (Config.UrlmenuInTitles()) {
                    TitleSupplementer->AddUrlmenuToTitle(SuperNaturalTitle);
                }
                if (Config.ForumsInTitles()) {
                    TitleSupplementer->AddForumToTitle(SuperNaturalTitle, shWrapper.GetForumsViewer());
                }
                if (Config.CatalogsInTitles()) {
                    TitleSupplementer->AddCatalogToTitle(SuperNaturalTitle);
                }
                if (Config.HostNameForUrlTitles() > 0) {
                    TitleSupplementer->AddHostNameToUrlTitle(SuperNaturalTitle);
                }
                if (Config.DefinitionForNewsTitles()) {
                    TitleSupplementer->AddDefinitionToNewsTitle(SuperNaturalTitle);
                }
                TitleSupplementer->AddHostDefinitionToTitle(SuperNaturalTitle);
                if (Config.AddPopularHostNameToTitle()) {
                    TitleSupplementer->AddPopularHostNameToTitle(SuperNaturalTitle);
                }
                if (Config.UrlRegionsInTitles()) {
                    TitleSupplementer->AddUrlRegionToTitle(SuperNaturalTitle);
                }
                if (Config.UserRegionsInTitles()) {
                    TitleSupplementer->AddUserRegionToTitle(SuperNaturalTitle);
                }
            } else {
                if (Config.BNASnippets()) {
                    TitleSupplementer->GenerateBNATitle(SuperNaturalTitle);
                }
            }
            if (Config.EraseBadSymbolsFromTitle()) {
                TitleSupplementer->EraseBadSymbolsFromTitle(SuperNaturalTitle);
            }
            if (needRemoveEmojis && CountEmojis(SuperNaturalTitle.GetTitleString()) > Config.AllowedTitleEmojiCount()) {
                TitleSupplementer->TransformTitle(SuperNaturalTitle, RemoveEmojis);
            }
            if (Config.ShouldCanonizeUnicode()) {
                TitleSupplementer->TransformTitle(SuperNaturalTitle, CanonizeUnicode);
            }
            if (Config.UseTurkey() && Config.CapitalizeEachWordTitleLetter()) {
                TitleSupplementer->CapitalizeWordTitleLetters(SuperNaturalTitle);
            }
        }

        Config.LogPerformance("FillReply.Replacers.Stop");
        Finalize();
        Config.LogPerformance("FillReply.Stop");
    }
};

TFillPassageReply::TFillPassageReply(TPassageReply& reply,
    const TConfig& cfg,
    const THitsInfoPtr hitsInfo,
    const TInlineHighlighter& ih,
    TArcManip& arcctx,
    const TQueryy& queryCtx,
    NUrlCutter::TRichTreeWanderer& richtreeWanderer)
{
    Impl.Reset(new TFillPassageReplyImpl(reply, cfg, hitsInfo, ih, arcctx, queryCtx, richtreeWanderer));
}


TFillPassageReply::~TFillPassageReply() {
}

void TFillPassageReply::FillReply() {
    Impl->FillReply();
}

}
