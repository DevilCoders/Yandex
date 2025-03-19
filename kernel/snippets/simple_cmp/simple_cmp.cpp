#include "simple_cmp.h"

#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <util/charset/wide.h>

namespace NSnippets {
namespace {
    class TSimpleSnipWeighter {
    private:
        const TQueryy& Query;
        TVector<bool> MatchedPositions;
        TVector<bool> MatchedSynPositions;

    public:
        TSimpleSnipWeighter(const TQueryy& query)
            : Query(query)
            , MatchedPositions(static_cast<size_t>(query.PosCount()), false)
            , MatchedSynPositions(static_cast<size_t>(query.PosCount()), false)
        {
        }

        void AddSnip(const TSnip& snip) {
            for (const TSingleSnip& ssnip : snip.Snips) {
                AddSpan(*ssnip.GetSentsMatchInfo(), ssnip.GetFirstWord(), ssnip.GetLastWord());
            }
        }

        void AddSpan(const TSentsMatchInfo& smi, int firstWord, int lastWord) {
            for (int wordId = firstWord; wordId <= lastWord; ++wordId) {
                for (int lemmaId : smi.GetNotExactMatchedLemmaIds(wordId)) {
                    for (int pos : Query.Id2Poss[lemmaId]) {
                        MatchedPositions[pos] = true;
                    }
                }
                for (int lemmaId : smi.GetSynMatchedLemmaIds(wordId)) {
                    for (int pos : Query.Id2Poss[lemmaId]) {
                        MatchedSynPositions[pos] = true;
                    }
                }
            }
        }

        double CalculateWeight(bool skipStopWords, bool useWizardWords) const {
            Y_ASSERT(MatchedPositions.size() == MatchedSynPositions.size());
            double weight = 0;
            for (size_t pos = 0; pos < MatchedPositions.size(); ++pos) {
                if (!MatchedPositions[pos]) {
                    continue;
                }

                // stop word case
                if (Query.Positions[pos].IsStopWord) {
                    if (!skipStopWords) {
                        weight += 0.1;
                    }
                    continue;
                }

                // user words only case
                if (!useWizardWords) {
                    if (Query.Positions[pos].IsUserWord) {
                        weight += 1.0;
                    }
                    continue;
                }

                // the most frequent synonyms case
                if (MatchedSynPositions[pos]) {
                    weight += 1.0;
                }
            }
            return weight;
        }

        bool ContainsAllQueryUserPositions(bool skipStopWords) const {
            for (size_t pos = 0; pos < MatchedPositions.size(); ++pos) {
                if (!MatchedPositions[pos] && Query.Positions[pos].IsUserWord &&
                    (!Query.Positions[pos].IsStopWord || !skipStopWords))
                {
                    return false;
                }
            }
            return true;
        }
    };

} // anonymous namespace

    TSimpleSnipCmp::TSimpleSnipCmp(const TQueryy& query, const TConfig& cfg, bool skipStopWords, bool useWizardWords)
        : Query(query)
        , Cfg(cfg)
        , SkipStopWords(skipStopWords)
        , UseWizardWords(useWizardWords)
    {
    }

    TSimpleSnipCmp& TSimpleSnipCmp::AddUrl(const TString& str) {
        Txt.push_back(UTF8ToWide(str));
        return *this;
    }

    TSimpleSnipCmp& TSimpleSnipCmp::Add(const TUtf16String& str) {
        Txt.push_back(str);
        return *this;
    }

    TSimpleSnipCmp& TSimpleSnipCmp::Add(const TSnip& snip) {
        for (const TSingleSnip& ssnip : snip.Snips) {
            Txt.push_back(ToWtring(ssnip.GetTextBuf()));
        }
        return *this;
    }

    TSimpleSnipCmp& TSimpleSnipCmp::Add(const TSnipTitle& title) {
        Txt.push_back(title.GetTitleString());
        return *this;
    }

    bool TSimpleSnipCmp::operator<=(const TSimpleSnipCmp& other) const {
        return GetWeight() <= other.GetWeight();
    }

    bool TSimpleSnipCmp::operator>=(const TSimpleSnipCmp& other) const {
        return GetWeight() >= other.GetWeight();
    }

    bool TSimpleSnipCmp::operator==(const TSimpleSnipCmp& other) const {
        return GetWeight() == other.GetWeight();
    }

    bool TSimpleSnipCmp::operator<(const TSimpleSnipCmp& other) const {
        return GetWeight() < other.GetWeight();
    }

    bool TSimpleSnipCmp::operator>(const TSimpleSnipCmp& other) const {
        return GetWeight() > other.GetWeight();
    }

    double TSimpleSnipCmp::GetWeight() const {
        TRetainedSentsMatchInfo customSents;
        customSents.SetView(Txt, TRetainedSentsMatchInfo::TParams(Cfg, Query));
        int wordsCount = customSents.GetSentsMatchInfo()->WordsCount();
        if (wordsCount == 0) {
            return 0.0;
        }
        TSimpleSnipWeighter weighter(Query);
        weighter.AddSpan(*customSents.GetSentsMatchInfo(), 0, wordsCount - 1);
        return weighter.CalculateWeight(SkipStopWords, UseWizardWords) + 1e-5;
    }

    bool TSimpleSnipCmp::ContainsAllQueryUserPositions() const {
        TRetainedSentsMatchInfo customSents;
        customSents.SetView(Txt, TRetainedSentsMatchInfo::TParams(Cfg, Query));
        TSimpleSnipWeighter weighter(Query);
        int wordsCount = customSents.GetSentsMatchInfo()->WordsCount();
        if (wordsCount > 0) {
            weighter.AddSpan(*customSents.GetSentsMatchInfo(), 0, wordsCount - 1);
        }
        return weighter.ContainsAllQueryUserPositions(SkipStopWords);
    }

    double TSimpleSnipCmp::CalcWeight(const TSnip& snip, const TSnip* titleSnip, bool skipStopWords, bool useWizardWords) {
        if (snip.Snips.empty()) {
            return 0.0;
        }
        TSimpleSnipWeighter weighter(snip.Snips.front().GetSentsMatchInfo()->Query);
        if (titleSnip) {
            weighter.AddSnip(*titleSnip);
        }
        weighter.AddSnip(snip);
        return weighter.CalculateWeight(skipStopWords, useWizardWords) + 1e-5;
    }
}
