#include "extended.h"
#include "extended_length.h"

#include <kernel/snippets/algo/extend.h>
#include <kernel/snippets/algo/maxfit.h>
#include <kernel/snippets/algo/redump.h>

#include <kernel/snippets/archive/chooser/chooser.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/iface/archive/segments.h>

#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/callback.h>

#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <kernel/segmentator/structs/segment_span.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

static constexpr int PRE_SNIPPET_SENTS = 2;
static constexpr int POST_SNIPPET_SENTS = 6;

static const TString EXPFLAG_SUPEREXT("superextended");
static const TString EXPFLAG_CONTENT_FRAG("extendwithcontent");
static const TString EXPFLAG_UNPACK_ALL("extsnip_unpack_all");
static const TString EXPFLAG_REMAP_CONTENT("remap_content");

bool IsFinalSnipEmpty(const TReplaceManager* manager)
{
    const TSnip* replacedSnip = manager->IsReplaced() ? manager->GetResult().GetSnip() : nullptr;
    if (!replacedSnip) {
        return manager->GetContext().Snip.Snips.empty();
    }
    else {
        return replacedSnip->Snips.empty();
    }
}

TSnip RemapSnip(const TSnip& snip, const TSentsMatchInfo& newSMI)
{
    TArcFragments arcSnip = NSnipRedump::SnipToArcFragments(snip, true);
    if (!arcSnip.Fragments) {
        return TSnip();
    }
    TSnip result = NSnipRedump::SnipFromArc(&newSMI, arcSnip);
    auto ii = snip.Snips.begin();
    auto jj = result.Snips.begin();
    for (; ii != snip.Snips.end() && jj != result.Snips.end(); ++ii, ++jj) {
        jj->SetForceDotsAtBegin(ii->GetForceDotsAtBegin());
        jj->SetForceDotsAtEnd(ii->GetForceDotsAtEnd());
        jj->SetAllowInnerDots(ii->GetAllowInnerDots());
    }
    return result;
}

void SelectWindowAroundSnippet(const TSnip& snip, const TArchiveView& extTextView, TArchiveView& selectedResult, int postLimit)
{
    TSentFilter choiceFilter(extTextView.Back()->SentId + 1);

    for (const TSingleSnip& ss : snip.Snips) {
        if (!ss.GetSentsMatchInfo()) {
            continue;
        }
        const TSentsInfo& info = ss.GetSentsMatchInfo()->SentsInfo;
        const auto& first = info.GetArchiveSent(ss.GetFirstSent());
        const auto& last = info.GetArchiveSent(ss.GetLastSent());
        if (first.SourceArc != ARC_TEXT || last.SourceArc != ARC_TEXT) {
            continue;
        }
        int firstId = first.SentId;
        int lastId = last.SentId;

        for (int j = firstId - PRE_SNIPPET_SENTS; j <= lastId + postLimit; ++j) {
            choiceFilter.Mark(j);
        }
    }

    for (size_t i = 0; i < extTextView.Size(); ++i) {
        const auto sent = extTextView.Get(i);
        size_t sentId = sent->SentId;
        if (choiceFilter.IsMarked(sentId)) {
            selectedResult.PushBack(sent);
        }
    }
}

NSegm::ESegmentType LookupSegType(const TSentsInfo& sentsInfo, int sentId)
{
    const NSegments::TSegmentsInfo* segments = sentsInfo.GetSegments();
    if (!segments || sentId >= sentsInfo.SentencesCount()) {
        return NSegm::STP_NONE;
    }
    return segments->GetType(segments->GetArchiveSegment(sentsInfo.GetArchiveSent(sentId)));
}

std::pair<int, int> FindContentSentRange(const TSentsInfo& si, int startFromSent)
{
    int firstContentSent = -1;
    int lastContentSent = -1;
    int prevArchiveSent = -1;

    for (int i = startFromSent; i < si.SentencesCount(); ++i) {
        int archId = si.GetArchiveSent(i).SentId;
        if (firstContentSent >= 0 && (archId > prevArchiveSent + 1 || archId < prevArchiveSent)) {
            break;
        }
        prevArchiveSent = archId;
        if (LookupSegType(si, i) == NSegm::STP_CONTENT) {
            if (firstContentSent < 0) {
                firstContentSent = i;
            }
            lastContentSent = i;
        } else if (firstContentSent >= 0) {
            break;
        }
    }

    return std::make_pair(firstContentSent, lastContentSent);
}

bool AddOneMoreGoodPassage(TSnip& extSnipCandidate, const TConfig& cfg, const TWordSpanLen& ruler, float maxLen)
{
    if (extSnipCandidate.Snips.size() != 1) {
        return false;
    }

    const TSingleSnip& firstFrag = extSnipCandidate.Snips.front();
    const TSentsInfo& si = firstFrag.GetSentsMatchInfo()->SentsInfo;

    std::pair<int, int> contentRange = FindContentSentRange(si, firstFrag.GetLastSent() + 1);

    if (contentRange.first < 0) {
        return false;
    }

    TSingleSnip secondFrag(
        si.FirstWordIdInSent(contentRange.first),
        si.LastWordIdInSent(contentRange.second),
        *firstFrag.GetSentsMatchInfo());

    float currentLen = ruler.CalcLength(firstFrag);
    float appendixLen = ruler.CalcLength(secondFrag);
    if (appendixLen < 1.0) {
        return false;
    }
    float remainingSpace = maxLen - currentLen - 0.5;
    if (currentLen < remainingSpace) {
        remainingSpace = currentLen;
    }
    if (remainingSpace > 0) {
        TSnip res = NMaxFit::GetTSnippet(cfg, ruler, secondFrag, remainingSpace);
        if (res.Snips.empty()) {
            return false;
        }
        extSnipCandidate.Snips.push_back(res.Snips.front());
        return true;
    }

    return false;
}

bool IsExtsnipLongEnough(const TSnip& natural, const TSnip& ext, const TWordSpanLen& ruler, bool experimentalMeasure)
{
    if (!ext.Snips) {
        return false;
    }

    if (experimentalMeasure) {
        float srcLen = ruler.CalcLength(natural.Snips);
        float extLen = ruler.CalcLength(ext.Snips);
        return (extLen - srcLen > 1.0 && extLen > srcLen * 1.3);
    } else {
        return (ext.WordsCount() * 10 > natural.WordsCount() * 13);
    }
}

TSnip GetExtendedIfLongEnough(const TReplaceContext& repCtx, float maxLen, const TSnip& source, const TSnip& natural, bool experimentalMeasure)
{
    TSnip extended;
    if (!source.Snips) {
        return extended;
    }

    const TSentsMatchInfo* smi = source.Snips.front().GetSentsMatchInfo();
    if (!smi) {
        return extended;
    }

    TSkippedRestr skipRestr(repCtx.IsByLink, smi->SentsInfo, repCtx.Cfg);
    extended = NSnippetExtend::GetSnippet(source, skipRestr, *smi, repCtx.SnipWordSpanLen, maxLen);
    if (!IsExtsnipLongEnough(natural, extended, repCtx.SnipWordSpanLen, experimentalMeasure)) {
        extended.Snips.clear();
    }

    return extended;
}

TSnip TryAlgorithms(const TSnip& snip, TReplaceManager& manager, const TArchiveView& extTextView)
{
    const TReplaceContext& repCtx = manager.GetContext();
    const TConfig& cfg = repCtx.Cfg;
    float maxLen = GetExtSnipLen(cfg, repCtx.LenCfg);

    // 1. Try appending a passage built from a content segment (if present & unpacked); measure using the new algorithm
    TSnip contentExtended;
    float contentExtLen = .0;

    if (!cfg.ExpFlagOff(EXPFLAG_CONTENT_FRAG)) {
        TSnip withContent = snip;

        if (cfg.ExpFlagOn(EXPFLAG_REMAP_CONTENT)) {
            TRetainedSentsMatchInfo& contSMI = manager.GetCustomSnippets().CreateRetainedInfo();
            contSMI.SetView(&repCtx.Markup, extTextView, TRetainedSentsMatchInfo::TParams(cfg, repCtx.QueryCtx).SetPutDot(true).SetMetaDescrAdd(&repCtx.SentsMInfo.SentsInfo.MetaDescrAdd));
            if (!withContent.Snips.empty()) {
                withContent = RemapSnip(withContent, *contSMI.GetSentsMatchInfo());
            }
        }

        if (AddOneMoreGoodPassage(withContent, cfg, repCtx.SnipWordSpanLen, maxLen)) {
            contentExtended = GetExtendedIfLongEnough(repCtx, maxLen, withContent, snip, true);
            contentExtLen = repCtx.SnipWordSpanLen.CalcLength(contentExtended.Snips);
        }
    }

    // 2. Build a good old extended snippet; measure using the old algorithm (require 30% more words)
    TSnip simpleExtended = GetExtendedIfLongEnough(repCtx, maxLen, snip, snip, false);
    float simpleExtLen = repCtx.SnipWordSpanLen.CalcLength(simpleExtended.Snips);

    // 3. Build an extended snippet from extra unpacked sents (if requested); measure with the new algo; use if longer than (2)
    bool wantSuperext = (!simpleExtended.Snips || cfg.ExpFlagOn(EXPFLAG_SUPEREXT));
    bool canSuperext = !cfg.ExpFlagOff(EXPFLAG_SUPEREXT) && extTextView.Size();
    if (wantSuperext && canSuperext) {
        TRetainedSentsMatchInfo& extSMI = manager.GetCustomSnippets().CreateRetainedInfo();
        TArchiveView selectedView;
        SelectWindowAroundSnippet(snip, extTextView, selectedView, cfg.ExpFlagOn(EXPFLAG_UNPACK_ALL) ? 100 : POST_SNIPPET_SENTS);
        extSMI.SetView(&repCtx.Markup, selectedView, TRetainedSentsMatchInfo::TParams(cfg, repCtx.QueryCtx).SetPutDot(true).SetMetaDescrAdd(&repCtx.SentsMInfo.SentsInfo.MetaDescrAdd));
        TSnip remappedSnip = RemapSnip(snip, *extSMI.GetSentsMatchInfo());
        TSnip superExtended = GetExtendedIfLongEnough(repCtx, maxLen, remappedSnip, snip, true);

        float superExtLen = repCtx.SnipWordSpanLen.CalcLength(superExtended.Snips);
        if (superExtLen > simpleExtLen && superExtLen > contentExtLen) {
            return superExtended;
        }
    }

    // A longer content-extended snippet from (1) is preferable to shorter snippet from (2)
    if (contentExtLen > simpleExtLen) {
        return contentExtended;
    }

    return simpleExtended;
}

bool DoPassages(TReplaceManager* manager, const TArchiveView& extTextView)
{
    const TReplaceContext& repCtx = manager->GetContext();
    const TConfig& cfg = repCtx.Cfg;

    if (repCtx.IsByLink) {
        return false;
    }

    const TSnip* repSnip = manager->GetResult().GetSnip();
    const TSnip& snip = repSnip ? *repSnip : repCtx.Snip;
    if (snip.Snips.empty() || manager->GetResult().HasCustomSnip()) {
        return false;
    }

    TSnip extended = TryAlgorithms(snip, *manager, extTextView);
    if (!extended.Snips) {
        return false;
    }

    if (cfg.NeedExtStat()) {
        manager->GetExtraSnipAttrs().AppendSpecAttr("extdw", ToString(extended.WordsCount() - snip.WordsCount()));
    }

    if (manager->GetCallback().GetDebugOutput()) {
        const THiliteMark oc(u"<b>", u"</b>");
        TVector<TZonedString> tmp = extended.GlueToZonedVec();
        TString snipStr;
        for (size_t i = 0; i < tmp.size(); ++i) {
            tmp[i].Zones[+TZonedString::ZONE_MATCH].Mark = &oc;
            snipStr += TGluer::GlueToHtmlEscapedUTF8String(TGluer::EmbedPara(tmp[i]));
        }
        manager->GetCallback().GetDebugOutput()->Print(false, "ExtSnip: %s", snipStr.data());
    }

    manager->GetExtraSnipAttrs().SetExtendedSnippet(extended);
    return true;
}

bool DoHeadline(TReplaceManager* manager)
{
    const TReplaceContext& repCtx = manager->GetContext();
    const TConfig& cfg = repCtx.Cfg;

    if (!manager->GetResult().GetTextExt().Long) {
        return false;
    }

    //TODO: Some replacers can preserve the link snippet for some reason
    //Assume we can't reliably extend such a snippet for now until the frontend is ready
    if (!cfg.ExpFlagOn("extsnips_allowed_by_link") && !IsFinalSnipEmpty(manager) && repCtx.IsByLink) {
        return false;
    }

    const TMultiCutResult& headline = manager->GetResult().GetTextExt();

    if (headline.CharCountDifference <= headline.CommonPrefixLen / 3.0) {
        return false;
    }
    if (cfg.NeedExtStat()) {
        manager->GetExtraSnipAttrs().AppendSpecAttr("extdw", ToString(headline.WordCountDifference));
    }
    if (manager->GetCallback().GetDebugOutput()) {
        TUtf16String tmp = headline.Long;
        EscapeHtmlChars<false>(tmp);
        manager->GetCallback().GetDebugOutput()->Print(false, "ExtHeadline: %s", WideToUTF8(tmp).data());
    }
    manager->GetExtraSnipAttrs().SetExtendedHeadline(headline);
    return true;
}

void TExtendedSnippetDataReplacer::DoWork(TReplaceManager* manager) {
    const TConfig& cfg = manager->GetContext().Cfg;

    if (!cfg.IsTouchReport() && !cfg.IsPadReport() && !cfg.IsWebReport() && !cfg.IsNeedExt()) {
        return;
    }

    if (cfg.SkipExtending()) {
        return;
    }

    if (manager->IsReplaced() && cfg.SwitchOffExtendedReplacerAnswer(manager->GetReplacerUsed())) {
        return;
    }

    // passages with the headline (extended or not) is an unsupported scenario
    if (manager->GetResult().GetText() && !IsFinalSnipEmpty(manager)) {
        return;
    }

    if (DoPassages(manager, ExtTextView)) {
        return;
    }

    if (!cfg.ExpFlagOn("no_extsnip_headline")) {
        DoHeadline(manager);
    }
}

}
