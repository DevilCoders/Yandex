#include "similarity.h"
#include "tsnip.h"
#include "sent_match.h"

#include <kernel/snippets/simple_textproc/deyo/deyo.h>

#include <util/generic/utility.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>

namespace NSnippets {
    static void InitWords(TVector<std::pair<TUtf16String, size_t>>& sortedWordFrq,
                          const TList<TSingleSnip>& snips) {
        size_t wordsCount = 0;
        for (const TSingleSnip& ssnip : snips) {
            wordsCount += ssnip.WordsCount();
        }
        THashMap<TUtf16String, size_t> wordFrq;
        wordFrq.reserve(wordsCount);
        for (const TSingleSnip& ssnip : snips) {
            const TSentsMatchInfo& smi = *ssnip.GetSentsMatchInfo();
            const int w0 = ssnip.GetFirstWord();
            const int w1 = ssnip.GetLastWord();
            for (int w = w0; w <= w1; ++w) {
                if (!smi.IsStopword(w)) {
                    TUtf16String word = smi.GetLowerWord(w);
                    DeyoInplace(word);
                    ++wordFrq[word];
                }
            }
        }
        sortedWordFrq.assign(wordFrq.begin(), wordFrq.end());
        Sort(sortedWordFrq);
    }

    static size_t CalcTotal(const TVector<std::pair<TUtf16String, size_t>>& sortedWordFrq) {
        size_t total = 0;
        for (const auto& word : sortedWordFrq) {
            total += word.second;
        }
        return total;
    }

    TEQInfo::TEQInfo(const TSnip& snip) {
        InitWords(SortedWordFrq, snip.Snips);
        Total = CalcTotal(SortedWordFrq);
    }

    TEQInfo::TEQInfo(const TSentsMatchInfo& smi, int w0, int w1) {
        InitWords(SortedWordFrq, TList<TSingleSnip>(1, TSingleSnip(w0, w1, smi)));
        Total = CalcTotal(SortedWordFrq);
    }

    size_t TEQInfo::CountEqWords(const TEQInfo& other) const {
        size_t eqCount = 0;
        size_t i = 0;
        size_t j = 0;
        while (i < SortedWordFrq.size() && j < other.SortedWordFrq.size()) {
            const auto& s = SortedWordFrq[i];
            const auto& t = other.SortedWordFrq[j];
            if (s.first == t.first) {
                eqCount += Min(s.second, t.second);
                ++i;
                ++j;
            } else if (s.first < t.first) {
                ++i;
            } else {
                ++j;
            }
        }
        return eqCount;
    }

    double GetSimilarity(const TEQInfo& a, const TEQInfo& b, bool normOnB) {
        size_t minLen = normOnB ? b.Total : Min(a.Total, b.Total);
        if (minLen == 0)
            return 0.0;
        return double(a.CountEqWords(b)) / minLen;
    }
}
