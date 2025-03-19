#include "internal_links.h"

#include "glue.h"
#include "tsnip.h"
#include "zones_helper.h"
#include "sent_match.h"

#include <kernel/snippets/archive/zone_checker/zone_checker.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/tarc/iface/tarcface.h>

#include <kernel/url_tools/url_tools.h>

#include <util/generic/algorithm.h>
#include <util/charset/unidata.h>

namespace NSnippets {

    static const int MAX_LEN_IN_WORDS = 5;
    static const size_t MAX_LEN_IN_CHARS = 40;
    static const int MIN_DIST_BETWEEN_LINKS = 10;

    class TExtSpan {
    public:
        TZonedString::TSpan Span;
        int BegWord, EndWord;
        int FragBegWord, FragEndWord;
        int MinDistToHit;
        int DistToSentBeg;
        TExtSpan(const TZonedString::TSpan& span, int begWord, int endWord, int fragBegWord, int fragEndWord,
            int minDistToHit, int distToSentBeg)
            : Span(span)
            , BegWord(begWord)
            , EndWord(endWord)
            , FragBegWord(fragBegWord)
            , FragEndWord(fragEndWord)
            , MinDistToHit(minDistToHit)
            , DistToSentBeg(distToSentBeg)
        {
        }

        TExtSpan()
            : Span()
            , BegWord()
            , EndWord()
            , FragBegWord()
            , FragEndWord()
            , MinDistToHit()
            , DistToSentBeg()
        {
        }

        bool operator<(const TExtSpan& r) const {
            if (this->BegWord != r.BegWord) {
                return this->BegWord < r.BegWord;
            }
            return this->EndWord < r.EndWord;
        }
    };

    struct THitLinksCMP {
        bool operator()(const TExtSpan& first, const TExtSpan& second) const {
            return first.operator<(second);
        }
    };

    struct TNonHitLinksCMP {
        bool operator()(const TExtSpan& first, const TExtSpan& second) const {
            if (first.MinDistToHit != second.MinDistToHit) {
                return first.MinDistToHit < second.MinDistToHit;
            }
            if (first.DistToSentBeg != second.DistToSentBeg) {
                return first.DistToSentBeg < second.DistToSentBeg;
            }
            return first.operator<(second);
        }
    };

    // link shortening - considering length, grammar info, etc

    static inline bool CanSplit(ESubTGrammar word, ESubTGrammar nextWord) {
        if (word == STG_NOUN && nextWord == STG_NOUN) {
            return false;
        }
        if (word == STG_ADJECTIVE && nextWord == STG_NOUN) {
            return false;
        }
        if (word == STG_VERB && nextWord == STG_VERB) {
            return false;
        }
        return true;
    }

    static inline bool IsGoodLength(const TSentsInfo& si, int beg, int end) {
        return end - beg + 1 <= MAX_LEN_IN_WORDS &&
            si.GetWordSpanBuf(beg, end).size() <= MAX_LEN_IN_CHARS;
    }

    static bool SpanHasQuotes(const TWtringBuf& span) {
        for (size_t i = 0; i < span.size(); ++i) {
            if (IsQuotation(span[i]) || IsSingleQuotation(span[i])) {
                return true;
            }
        }
        return false;
    }

    static void MakeSpanFit(const TSentsMatchInfo* smInfo, int& beg, int& end) {
        const TSentsInfo& si = smInfo->SentsInfo;

        // If the span contains any quote - it goes to hell!
        if (SpanHasQuotes(si.GetWordSpanBuf(beg, end))) {
            beg = end + 1;
            return;
        }

        int initEnd = end;
        while (beg <= end && !(IsGoodLength(si, beg, end) && smInfo->PunctBalInGapInsideRange(beg, end) == 0)) {
            --end;
        }
        if (beg > end) {
            return;
        }
        if (initEnd == end) { // There was no shortening - the initial span was fine
            return;
        } else {
            end = initEnd; // Restore the end
        }

        // We want to capture the first hit word and further. If there is no hits, we start form the first (beg) word
        int hitWordId = beg;
        for (int i = beg; i <= end; ++i) {
            if (smInfo->MatchesInRange(i) > 0) {
                hitWordId = i;
                break;
            }
        }

        int maxMatches = -1;
        int bestBeg = -1;
        int bestEnd = -1;
        // Here we try to maximize hits in link; hitWordId should be coverered with the link
        for (int newBeg = Max(beg, hitWordId - MAX_LEN_IN_WORDS); newBeg <= hitWordId; ++newBeg) {
            if (newBeg > beg) {
                if (si.WordVal[newBeg].IsSuffix) {
                    continue;
                }
                if (smInfo->PunctGapsInRange(newBeg - 1, newBeg) == 0) { // -1 is safe
                    ESubTGrammar afterSplitGrammar = smInfo->GetWordSubGrammar(newBeg);
                    if (afterSplitGrammar == STG_BAD_POS) {
                        continue;
                    }
                    ESubTGrammar beforeSplitGrammar = smInfo->GetWordSubGrammar(newBeg - 1);
                    if (!CanSplit(beforeSplitGrammar, afterSplitGrammar)) {
                        continue;
                    }
                }
            }

            for (int len = MAX_LEN_IN_WORDS; len >= 3; --len) {
                int newEnd = newBeg + len - 1;
                if (newEnd > end) {
                    continue;
                }
                if (!(IsGoodLength(si, newBeg, newEnd) && smInfo->PunctBalInGapInsideRange(newBeg, newEnd) == 0)) {
                    continue;
                }
                // Hereinafter urls in anchor are bad
                TIsUrlResult isUrl(IsUrl(WideToUTF8(si.GetWordSpanBuf(newBeg, newEnd)), 0));
                if (isUrl.IsUrl) {
                    break; // not continue, because if the string is an URL, than its substring is considered as url too
                }
                if (newEnd < end) {
                    if (si.WordVal[newEnd + 1].IsSuffix) { // +1 is safe
                        continue;
                    }
                    if (smInfo->PunctGapsInRange(newEnd, newEnd + 1) == 0) {
                        ESubTGrammar beforeSplitGrammar = smInfo->GetWordSubGrammar(newEnd);
                        if (beforeSplitGrammar == STG_BAD_POS) {
                            continue;
                        }
                        ESubTGrammar afterSplitGrammar = smInfo->GetWordSubGrammar(newEnd + 1);
                        if (!CanSplit(beforeSplitGrammar, afterSplitGrammar)) {
                            continue;
                        }
                    }
                }
                if (smInfo->MatchesInRange(newBeg, newEnd) > maxMatches) {
                    maxMatches = smInfo->MatchesInRange(newBeg, newEnd);
                    bestBeg = newBeg;
                    bestEnd = newEnd;
                }
            }
        }
        if (bestBeg == -1 || bestEnd == -1) {
            beg = end + 1; // We didnt' build any good links
        } else {
            // Save the new borders of the link
            beg = bestBeg;
            end = bestEnd;
        }
    }

    // links retrieving

    static void CollectLinks(const TSingleSnip* snip, TVector<TExtSpan>& matchedLinks) {
        const TSentsInfo& si = snip->GetSentsMatchInfo()->SentsInfo;
        TVector<TZoneWords> borders;
        GetZones(*snip, AZ_ANCHORINT, borders, true);

        const TArchiveMarkupZones& zones = si.GetTextArcMarkup();
        const TArchiveZoneAttrs& attrs = zones.GetZoneAttrs(AZ_ANCHORINT);

        int w0 = snip->GetFirstWord();
        int w1 = snip->GetLastWord();
        for (size_t i = 0; i < borders.size(); ++i) {
            const TArchiveZoneSpan& zSpan = borders[i].Span;
            // retrieve linkint attrs
            const THashMap<TString, TUtf16String>* spanAttrs = attrs.GetSpanAttrs(zSpan).AttrsHash;
            if (!spanAttrs) {
                continue;
            }
            const char* attrName = NArchiveZoneAttr::NAnchorInt::SEGMLINKINT;
            THashMap<TString, TUtf16String>::const_iterator attrIter = spanAttrs->find(attrName);
            if (attrIter == spanAttrs->end()) {
                continue;
            }

            // build span
            int wordBeg = borders[i].FirstWordId;
            int wordEnd = borders[i].LastWordId;
            MakeSpanFit(snip->GetSentsMatchInfo(), wordBeg, wordEnd);
            if (wordBeg > wordEnd) {
                continue;
            }
            TZonedString::TSpan span(si.GetWordSpanBuf(wordBeg, wordEnd));
            span.AddAttr(attrName, attrIter->second);

            // put span
            if (snip->GetSentsMatchInfo()->MatchesInRange(wordBeg, wordEnd) > 0) { // the link contain hit[s]
                int sentId = si.WordId2SentId(w0);
                int firstWordId = si.FirstWordIdInSent(sentId);
                matchedLinks.push_back(TExtSpan(span, wordBeg, wordEnd, w0, w1, i, w0 - firstWordId));
            }
        }
    }

    // Links selecting: 1) 2 links tops; 2) They should not be very close to each other

    static inline bool AreGoodSortedSpans(const TExtSpan& lSpan, const TExtSpan& rSpan) {
        // lSpan.BegWord <= rSpan.BegWord <= rSpan.EndWord
        if (rSpan.BegWord <= lSpan.EndWord) {
            return false;
        }

        // There is no need to show two identical links
        THashMap<TString, TUtf16String>::const_iterator it1 = lSpan.Span.Attrs.find(NArchiveZoneAttr::NAnchorInt::SEGMLINKINT);
        THashMap<TString, TUtf16String>::const_iterator it2 = rSpan.Span.Attrs.find(NArchiveZoneAttr::NAnchorInt::SEGMLINKINT);
        if (it1->second == it2->second) {
            return false;
        }

        // minimal distance between links should be equal or more than 10 words
        if (lSpan.FragBegWord == rSpan.FragBegWord && lSpan.FragEndWord == rSpan.FragEndWord) {
            // handle with links in one fragment
            return rSpan.BegWord - lSpan.EndWord - 1 >= MIN_DIST_BETWEEN_LINKS;
        } else {
            int fromEnd = lSpan.FragEndWord - lSpan.EndWord;
            int fromBeg = rSpan.BegWord - rSpan.FragBegWord;
            return fromBeg + fromEnd >= MIN_DIST_BETWEEN_LINKS;
        }
    }

    static inline bool AreGoodSpans(const TExtSpan& lSpan, const TExtSpan& rSpan) {
        if (rSpan < lSpan) {
            return AreGoodSortedSpans(rSpan, lSpan);
        }
        return AreGoodSortedSpans(lSpan, rSpan);
    }

    static inline bool PutBothSpans(const TExtSpan& span1, const TExtSpan& span2, TVector<TZonedString::TSpan>& spans) {
        if (AreGoodSpans(span1, span2)) {
            spans.push_back(span1.Span);
            spans.push_back(span2.Span);
            return true;
        }
        return false;
    }

    static inline void AddFirstAndLast(const TVector<TExtSpan>& extSpans, TVector<const TExtSpan*>& outSpans) {
        if (extSpans.size() > 0) {
            outSpans.push_back(&*extSpans.begin());
            if (extSpans.size() > 1) {
                outSpans.push_back(&*extSpans.rbegin());
            }
        }
    }

    static void GetTwoOrLessSpans(const TVector<TExtSpan>& matchedSpan, TVector<TZonedString::TSpan>& spans) {
        spans.clear();
        TVector<const TExtSpan*> mixedSpans;
        AddFirstAndLast(matchedSpan, mixedSpans);
        Y_ASSERT(mixedSpans.size() <= 2);
        // Try to put exact two matched spans
        if (mixedSpans.size() == 2) {
            if (PutBothSpans(*mixedSpans[0], *mixedSpans[1], spans)) {
                return;
            }
        }

        // Try to put one span
        if (!mixedSpans.empty()) {
            spans.push_back((*mixedSpans.begin())->Span);
        }
    }

    // links filtering
    static void FilterOutSpans(TVector<TExtSpan>& spans) {
        size_t n = 0;
        // filter out spans with ...
        for (size_t i = 0; i < spans.size(); ++i) {
            const TZonedString::TSpan& zone = spans[i].Span;
            // ... too short anchor text
            if (zone.Span.size() < 3) {
                continue;
            }

            // ... links in anchor
            TIsUrlResult isUrl(IsUrl(WideToUTF8(zone.Span), 0));
            if (isUrl.IsUrl) {
                continue;
            }

            // ... no alphas or capitalized
            bool containsAlpha = false;
            bool spanIsCapitalized = true;
            for (size_t pos = 0; pos < zone.Span.size(); ++pos) {
                if (IsAlpha(*(zone.Span.data() + pos))) {
                    containsAlpha = true;
                }
                if (IsLower(*(zone.Span.data() + pos))) {
                    spanIsCapitalized = false;
                }
            }
            if (!containsAlpha) {
                continue;
            }
            if (spanIsCapitalized) {
                continue;
            }

            THashMap<TString, TUtf16String>::const_iterator attrsIter = zone.Attrs.find(NArchiveZoneAttr::NAnchorInt::SEGMLINKINT);
            if (attrsIter == zone.Attrs.end()) { // ... no attrs (?)
                continue;
            }
            if (attrsIter->second.empty()) { // ... empty links
                continue;
            }

            DoSwap(spans[n], spans[i]);
            ++n;
        }
        spans.resize(n);
    }

    void GetLinkSpans(const TSnip& snip, TVector<TZonedString::TSpan>& spans) {
        spans.clear();

        TVector<TExtSpan> matchedLinks;
        for (TSnip::TSnips::const_iterator it = snip.Snips.begin(); it != snip.Snips.end(); ++it) {
            CollectLinks(&*it, matchedLinks);
        }

        FilterOutSpans(matchedLinks);

        // Select only two links; links with matches are the best
        Sort(matchedLinks.begin(), matchedLinks.end(), THitLinksCMP());
        GetTwoOrLessSpans(matchedLinks, spans);
    }

} // namespace NSnippets

