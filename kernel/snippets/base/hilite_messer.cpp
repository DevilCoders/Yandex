#include "hilite_messer.h"
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include "pseudo_rand.h"

#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/passagereply.h>

#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/snippets/strhl/hilite_mark.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/vector.h>
#include <util/random/mersenne.h>
#include <util/random/shuffle.h>

namespace NSnippets {
    namespace {
        void Push(TVector<int>& to, TVector<int>& from) {
            for (size_t i = 0; i < from.size(); ++i) {
                to.push_back(from[i]);
            }
        }

        TVector<bool> GetRandVec(const int matchWords, const TSentsMatchInfo& smInfo,
                size_t pseudoRand, bool addHilite) {
            TVector<int> nonMatchedId;
            TVector<int> matchedId;
            for (int i = 0; i < smInfo.WordsCount(); ++i) {
                if (smInfo.IsMatch(i)) {
                    matchedId.push_back(i);
                } else {
                    nonMatchedId.push_back(i);
                }
            }

            TMersenne<ui64> mersenn(pseudoRand);
            Shuffle(nonMatchedId.begin(), nonMatchedId.end(), mersenn);
            Shuffle(matchedId.begin(), matchedId.end(), mersenn);
            TVector<bool> res(smInfo.WordsCount(), false);

            if (addHilite) { // double the highlighting (in average)
                for (size_t i = 0; i < matchedId.size(); ++i) {
                    res[matchedId[i]] = true;
                }
                size_t mod = (size_t)(nonMatchedId.size() / 3.);
                size_t add = mod == 0 ? 0 : mersenn.GenRand() % mod;
                for (size_t i = 0; i < add; ++i) {
                    res[nonMatchedId[i]] = true;
                }
            } else {
                TVector<int> total;
                Push(total, nonMatchedId);
                Push(total, matchedId);
                for (int i = 0; i < matchWords; ++i) {
                    res[total[i]] = true;
                }
            }
            return res;
        }

        TUtf16String Hilite(const TSentsMatchInfo& smInfo, const int matchWords, size_t pseudoRand,
                bool addHilite) {
            const TSentsInfo& si = smInfo.SentsInfo;
            TVector<bool> messedMatch = GetRandVec(matchWords, smInfo, pseudoRand, addHilite);
            // if messedMatch[i] = true, i-th words schould be highlighted
            TUtf16String res;
            for (size_t i = 0; i < messedMatch.size(); ++i) {
                res += si.GetBlanksBefore(i);
                if (messedMatch[i]) {
                    res += DEFAULT_MARKS.OpenTag;
                }
                res += si.GetWordBuf(i);
                if (messedMatch[i]) {
                    res += DEFAULT_MARKS.CloseTag;
                }
            }
            res += si.GetBlanksAfter(messedMatch.size() - 1);
            return res;
        }

        int CountMatchesAndClear(const TUtf16String& frag, TUtf16String& clearFrag) {
            int matchWords = 0;
            clearFrag.clear();
            for (size_t i = 0; i < frag.size(); ++i) {
                if (frag[i] == 0x07 && i + 1 < frag.size()) {
                    if (frag[i + 1] == '[' || frag[i + 1] == ']') {
                        ++matchWords;
                        ++i;
                        // skip doubled highlighting
                        if (i + 2 < frag.size() && frag[i + 1] == 0x07 && frag[i + 2] == frag[i]) {
                            ++i;
                        }
                        continue;
                    }
                }
                clearFrag.push_back(frag[i]);
            }
            Y_ASSERT(matchWords % 2 == 0);
            return matchWords / 2;
        }

        TUtf16String GetMessedHilite(const TUtf16String& hilitedFrag, const TConfig& cfg, const TQueryy& query,
                size_t pseudoRand, bool addHilite) {
            TUtf16String clearFrag;
            int matchWords = CountMatchesAndClear(hilitedFrag, clearFrag);

            TRetainedSentsMatchInfo customSents;
            customSents.SetView(clearFrag, TRetainedSentsMatchInfo::TParams(cfg, query));
            const TSentsMatchInfo& smInfo = *customSents.GetSentsMatchInfo();
            if (smInfo.WordsCount() == 0) {
                return clearFrag;
            }
            TUtf16String res = Hilite(smInfo, matchWords, pseudoRand, addHilite);
            if (!res.empty() && IsSpace(res.back())) {
                res.pop_back();
            }
            return res;
        }

        TVector<TUtf16String> GetMessedHilite(const TVector<TUtf16String>& hilitedSnipVec, const TConfig& cfg,
                const TQueryy& query, size_t pseudoRand, bool addHilite) {
            TVector<TUtf16String> res;
            for (size_t i = 0; i < hilitedSnipVec.size(); ++i) {
                res.push_back(GetMessedHilite(hilitedSnipVec[i], cfg, query, pseudoRand, addHilite));
                pseudoRand = NumericHash(pseudoRand);
            }
            return res;
        }

        void MessWithSnip(const TConfig& cfg, const TQueryy& queryCtx,
                TPassageReply& reply, bool addHilite, size_t pseudoRand) {
            pseudoRand = NumericHash(pseudoRand);
            TVector<TUtf16String> messedSnipVec = GetMessedHilite(reply.GetPassages(),
                    cfg, queryCtx, pseudoRand, addHilite);
            reply.SetPassages(messedSnipVec);

            pseudoRand = NumericHash(pseudoRand);
            TUtf16String messedTitle = GetMessedHilite(reply.GetTitle(), cfg,
                    queryCtx, pseudoRand, addHilite);
            reply.SetTitle(messedTitle);

            pseudoRand = NumericHash(pseudoRand);
            TUtf16String messedMeta = GetMessedHilite(reply.GetHeadline(), cfg,
                    queryCtx, pseudoRand, addHilite);
            reply.SetHeadline(messedMeta);
        }
    }

    void MessWithReply(const TConfig& cfg, const TQueryy& queryCtx, const TString& url,
            const THitsInfoPtr& hitsInfo, TPassageReply& reply) {
        size_t pseudoRand = PseudoRand(url, hitsInfo);
        if (cfg.WantToMess(pseudoRand)) {
            if (cfg.NoHilite()) {
                TUtf16String tmpWtr;
                TVector<TUtf16String> tmpVec;
                for (size_t i = 0; i < reply.GetPassages().size(); ++i) {
                    CountMatchesAndClear(reply.GetPassages()[i], tmpWtr);
                    tmpVec.push_back(tmpWtr);
                }
                reply.SetPassages(tmpVec);
                CountMatchesAndClear(reply.GetTitle(), tmpWtr);
                reply.SetTitle(tmpWtr);
                CountMatchesAndClear(reply.GetHeadline(), tmpWtr);
                reply.SetHeadline(tmpWtr);
            } else {
                if (cfg.ShuffleHilite()) {
                    MessWithSnip(cfg, queryCtx, reply, false, pseudoRand);
                } else if (cfg.AddHilite()) {
                    MessWithSnip(cfg, queryCtx, reply, true, pseudoRand);
                }
            }
        }
    }
}
