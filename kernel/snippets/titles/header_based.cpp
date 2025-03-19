#include "header_based.h"

#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/zone_checker/zone_checker.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/wordstat/wordstat.h>
#include <kernel/snippets/wordstat/wordstat_data.h>

#include <util/generic/string.h>
#include <util/generic/algorithm.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets
{
    static const int MAX_SENT_COUNT = 25;
    static const int MAX_TOTAL_SENT_LEN = 7000;

    THeaderViewer::THeaderViewer()
      : MaxSentCount(MAX_SENT_COUNT)
      , MaxTotalSentLen(MAX_TOTAL_SENT_LEN)
      , Unpacker(nullptr)
    {
    }
    void THeaderViewer::OnUnpacker(TUnpacker* unpacker) {
        Unpacker = unpacker;
    }

    void THeaderViewer::FilterSpans(const TVector<TArchiveZoneSpan>& headerSpans,
                                    const TVector<TArchiveZoneSpan>& forbiddenSpans,
                                    TVector<TArchiveZoneSpan>& filteredSpans)
    {
        TForwardInZoneChecker headerZoneChecker = TForwardInZoneChecker(forbiddenSpans, true);
        filteredSpans.reserve(MaxSentCount);
        int sentCount = 0;

        for (size_t i = 0;  i < headerSpans.size(); ++i) {
            headerZoneChecker.SeekToSent(headerSpans[i].SentBeg);
            const TArchiveZoneSpan* curSpan = headerZoneChecker.GetCurrentSpan();
            if (!headerZoneChecker.Empty() && headerSpans[i].SentBeg >= curSpan->SentBeg && headerSpans[i].SentEnd <= curSpan->SentEnd)
            {
                continue;
            }
            TArchiveZoneSpan span = headerSpans[i];
            int n = span.SentEnd - span.SentBeg + 1;
            if (sentCount + n > MaxSentCount) {
                n = MaxSentCount - sentCount;
                span.SentEnd = span.SentBeg + n - 1;
            }
            filteredSpans.push_back(span);
            sentCount += n;
            if (sentCount >= MaxSentCount) {
                break;
            }
        }
    }

    void THeaderViewer::OnMarkup(const TArchiveMarkupZones& zones) {
        TVector<TArchiveZoneSpan> headerSpans = zones.GetZone(AZ_HEADER).Spans;
        SortZones(headerSpans);

        TVector<TArchiveZoneSpan> forbiddenSpans;
        forbiddenSpans.insert(forbiddenSpans.end(), zones.GetZone(AZ_SEGCOPYRIGHT).Spans.begin(), zones.GetZone(AZ_SEGCOPYRIGHT).Spans.end());
        forbiddenSpans.insert(forbiddenSpans.end(), zones.GetZone(AZ_SEGAUX).Spans.begin(), zones.GetZone(AZ_SEGAUX).Spans.end());
        forbiddenSpans.insert(forbiddenSpans.end(), zones.GetZone(AZ_SEGMENU).Spans.begin(), zones.GetZone(AZ_SEGMENU).Spans.end());
        forbiddenSpans.insert(forbiddenSpans.end(), zones.GetZone(AZ_ANCHOR).Spans.begin(), zones.GetZone(AZ_ANCHOR).Spans.end());
        forbiddenSpans.insert(forbiddenSpans.end(), zones.GetZone(AZ_ANCHORINT).Spans.begin(), zones.GetZone(AZ_ANCHORINT).Spans.end());

        TVector<TArchiveZoneSpan> filteredSpans;
        FilterSpans(headerSpans, forbiddenSpans, filteredSpans);

        for (size_t i = 0; i < filteredSpans.size(); ++i) {
            All.PushBack(filteredSpans[i].SentBeg, filteredSpans[i].SentEnd);
        }
        Unpacker->AddRequest(All);
    }

    void THeaderViewer::OnEnd() {
        DumpResult(All, Result);
        int len = 0;
        for (size_t i = 0; i < Result.Views.size(); ++i) {
            for (size_t j = 0; j < Result.Views[i].Size(); ++j) {
                len += Result.Views[i].Get(j)->Sent.size();
                if (len > MaxTotalSentLen) {
                    Result.Views[i].Resize(j + 1);
                    Result.Views.resize(i + 1);
                    return;
                }
            }
        }
    }

    static bool IsWikipediaHost(const TString& url)
    {
        return GetOnlyHost(url).EndsWith(".wikipedia.org");
    }

    static bool IsOtvetMailHost(const TString& url)
    {
        return GetOnlyHost(url) == "otvet.mail.ru";
    }

    static bool TitleContainsAllQueryWords(const TQueryy& query, const TSnipTitle& naturalTitle)
    {
        TWordStatData naturalTitleStatData(query, naturalTitle.GetSentsInfo()->MaxN);
        const TSnip& titleSnip = naturalTitle.GetTitleSnip();
        for (TSnip::TSnips::const_iterator it = titleSnip.Snips.begin(); it != titleSnip.Snips.end(); ++it) {
            naturalTitleStatData.PutSpan(*it->GetSentsMatchInfo(), it->GetFirstWord(), it->GetLastWord());
        }
        return naturalTitleStatData.LikeWordSeenCount.NonstopUser >= query.NonstopUserPosCount();
    }

    static bool CompareTitleCandidates(const TSnipTitle& title1, const TSnipTitle& title2) {
        if (title1.GetSynonymsCount() > title2.GetSynonymsCount()) {
            return true;
        }
        return  title1.GetSynonymsCount() == title2.GetSynonymsCount() &&
                title1.GetLogMatchIdfSum() + 1E-7 > title2.GetLogMatchIdfSum();
    }

    static size_t GetCommonPrefixLen(const TUtf16String& s1, const TUtf16String& s2) {
        size_t res = 0;
        size_t limit = Min(s1.size(), s2.size());
        while (res < limit && s1[res] == s2[res]) {
            res++;
        }
        return res;
    }


    static constexpr double MIN_HEADER_SIMILARITY_RATIO = 0.9;

    bool GenerateHeaderBasedTitle(TSnipTitle& resTitle, const THeaderViewer& viewer, const TQueryy& query, const TSnipTitle& naturalTitle,
                                  const TMakeTitleOptions& options, const TConfig& config, const TString& url, ISnippetCandidateDebugHandler* candidateHandler)
    {
        if (config.IsNeedGenerateTitleOnly()) {
            return false;
        }
        if (config.IsUrlOrEmailQuery()) {
            return false;
        }
        if (IsWikipediaHost(url) || IsOtvetMailHost(url)) {
            return false;
        }
        if (!config.EliminateDefinitions() && TitleContainsAllQueryWords(query, naturalTitle)) {
            return false;
        }
        TSnipTitle bestTitle;
        bool glued = false;
        size_t iters = 1;
        if (!naturalTitle.GetTitleString().empty() && naturalTitle.GetPixelLength() < 0.7 * options.PixelsInLine) {
            iters = 2;
        }
        const bool altheaders3 = !config.ExpFlagOff("altheaders3"); // SNIPPETS-2979
        TUtf16String naturalTitleStringToGlue(naturalTitle.GetTitleString());
        if (altheaders3) {
            EraseFullStop(naturalTitleStringToGlue);
        }
        size_t bestTitlePrefixLen = 0;
        bool foundPrefixTitle = false;
        for (size_t glue = 0; glue < iters; ++glue) {
            for (size_t i = 0; i < viewer.Result.Views.size(); ++i) {
                const TArchiveView& candidate = viewer.Result.Views[i];
                TUtf16String titleString;
                for (size_t j = 0; j < candidate.Size(); ++j) {
                    TUtf16String sent(candidate.Get(j)->Sent);
                    ClearChars(sent, /* allowSlash */ false, config.AllowBreveInTitle());
                    titleString += sent;
                    if (j + 1 < candidate.Size())
                        titleString.push_back(' ');
                }
                if (glue) {
                    if (SimilarTitleStrings(naturalTitleStringToGlue, titleString, MIN_HEADER_SIMILARITY_RATIO)) {
                        continue;
                    }
                    titleString = naturalTitleStringToGlue + GetTitleSeparator(config) + titleString;
                }
                TMakeTitleOptions newOptions = options;
                newOptions.DefinitionMode = TDM_IGNORE;
                if (glue) {
                    newOptions.TitleGeneratingAlgo = TGA_NAIVE;
                }
                TSnipTitle title = MakeTitle(titleString, config, query, newOptions);

                if (candidateHandler) {
                     candidateHandler->AddTitleCandidate(title, glue ? TS_NATURAL_WITH_HEADER_BASED : TS_HEADER_BASED);
                }

                if (config.EliminateDefinitions()) {
                    // true if the same - it's a way to find out that original title is good
                    if (!title.GetTitleString().StartsWith(naturalTitle.GetTitleString()) || title.GetTitleString() == naturalTitle.GetTitleString()) {
                        size_t titlePrefixLen = GetCommonPrefixLen(title.GetTitleString(), naturalTitle.GetTitleString());

                        if (titlePrefixLen > bestTitlePrefixLen && config.IsTitleContractionOk(naturalTitle.GetTitleString().size(), titlePrefixLen)) {
                            bestTitle = title;
                            bestTitlePrefixLen = titlePrefixLen;
                            foundPrefixTitle = true;
                            if (glue) {
                                 glued = true;
                            }
                        }
                    }
                }
                if (!foundPrefixTitle && CompareTitleCandidates(title, bestTitle)) {
                    bestTitle = title;
                    if (glue) {
                         glued = true;
                    }
                }
            }
        }

        if (naturalTitle.GetTitleString().empty()) {
            if (!bestTitle.GetTitleString().empty()) {
                resTitle = bestTitle;
                return true;
            }
            return false;
        }

        bool forbiddenExtending = config.EliminateDefinitions() &&
            bestTitle.GetTitleString().StartsWith(naturalTitle.GetTitleString()) &&
            bestTitle.GetTitleString() != naturalTitle.GetTitleString();

        if (foundPrefixTitle || config.IsNeedGenerateAltHeaderOnly() ||
            IsCandidateBetterThanTitle(query, bestTitle, naturalTitle, altheaders3, glued) && !forbiddenExtending)
        {
            resTitle = bestTitle;
            return true;
        } else {
            return false;
        }
    }
}
