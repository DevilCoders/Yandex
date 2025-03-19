#include "title_trigram.h"

#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/similarity.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NSnippets {
    class TTrigramHashIterator {
    public:
        using THashVal = size_t;
        static const ui32 PRIME = 16777619U;

        THashVal CurrentHash = 0;
        THashVal Window0 = 0, Window1 = 0, Window2 = 0;
        int NWords = 0;
        bool CurrentWordIsGood = false;

        THashVal Next(const TSentsMatchInfo& smi, int wid) {
            if (!smi.SentsInfo.WordVal[wid].IsSuffix) {
                CurrentHash = 0;
                CurrentWordIsGood = !smi.IsStopword(wid);
            }

            if (CurrentWordIsGood) {
                const TUtf16String& canonicalWord = smi.GetLowerWord(wid);
                CurrentHash = (CurrentHash * PRIME) ^ HashVal(canonicalWord.data(), canonicalWord.size());
            }

            if (CurrentWordIsGood && (wid == smi.WordsCount() - 1 || !smi.SentsInfo.WordVal[wid + 1].IsSuffix)) {
                CurrentWordIsGood = false;
                THashVal result = UpdateHash(CurrentHash);
                return NWords > 2 ? result : 0;
            }
            return 0;
        }

        inline THashVal HashVal(const wchar16* ptr, size_t len) {
            return ComputeHash(TWtringBuf{ptr, len});
        }

        inline THashVal UpdateHash(THashVal hash) {
            Window2 = (Window1 * PRIME) ^ hash;
            Window1 = (Window0 * PRIME) ^ hash;
            Window0 = hash;
            ++NWords;
            return Window2 != 0 ? Window2 : 1;
        }
    };

    class TTitleTrigramMatcher {
    private:
        using THashVal = TTrigramHashIterator::THashVal;
        TVector<THashVal> Hashes;
        TTrigramHashIterator Iterator;
        bool EmptyTitle = true;
        bool PrevMatch = false;

    public:
        explicit TTitleTrigramMatcher(const TSnipTitle* title) {
            Hashes.resize(256, 0);
            InitTitleHashes(title);
        }

        bool IsEmptyTitle() const {
            return EmptyTitle;
        }

    private:
        // Add values in non-decreasing order, please
        void AddTrigram(THashVal val)
        {
            THashVal pos = val >> (8 * (sizeof(THashVal) - 1));
            THashVal limit = pos + 256;
            while (pos < limit) {
                THashVal& item = Hashes[pos & 0xFF];
                if (item == 0 || item == val) {
                    item = val;
                    return;
                }
                ++pos;
            }
        }

        bool HasTrigram(THashVal val)
        {
            THashVal pos = val >> (8 * (sizeof(THashVal) - 1));
            THashVal limit = pos + 256;
            while (pos < limit) {
                THashVal item = Hashes[pos & 0xFF];
                if (item >= val || item == 0) {
                    return (item == val);
                }
                ++pos;
            }
            return false;
        }

        void InitTitleHashes(const TSnipTitle* title)
        {
            if (!title) {
                return;
            }
            TVector<THashVal> tmpHashes;
            TTrigramHashIterator hasher;
            for (const TSingleSnip& ssnip : title->GetTitleSnip().Snips) {
                for (int i = ssnip.GetFirstWord(); i <= ssnip.GetLastWord(); ++i) {
                    THashVal hash = hasher.Next(*ssnip.GetSentsMatchInfo(), i);
                    if (hash) {
                        tmpHashes.push_back(hash);
                    }
                }
            }
            EmptyTitle = tmpHashes.empty();
            Sort(tmpHashes.begin(), tmpHashes.end());
            for (THashVal hash : tmpHashes) {
                AddTrigram(hash);
            }
        }

    public:
        void SetStart() {
            PrevMatch = false;
        }

        std::pair<bool, bool> MatchStep(const TSentsMatchInfo& smi, int wid) {
            THashVal hash = Iterator.Next(smi, wid);
            bool hasMatch = hash && HasTrigram(hash);
            std::pair<bool, bool> retval = {PrevMatch && hasMatch, hasMatch};
            if (hash) {
                PrevMatch = hasMatch;
            }
            return retval;
        }
    };

    int TTitleMatchInfo::GetRawTitleWordCover(int i, int j) const {
        int matches = WordAcc[j + 1].SumTitleMatches - WordAcc[i].SumTitleMatches;
        int discMatches = WordAcc[j + 1].SumDiscountedTitleMatches - WordAcc[i].SumDiscountedTitleMatches;
        if (WordAcc[i + 1].SumDiscountedTitleMatches > WordAcc[i].SumDiscountedTitleMatches) {
            discMatches--;
        }
        return 3 * matches - 2 * discMatches;
    }

    void TTitleMatchInfo::Fill(const TSentsMatchInfo& text, const TSnipTitle* title) {
        //title trigram matching
        WordVal.resize(text.WordsCount());
        WordAcc.resize(text.WordsCount() + 1);
        TTitleTrigramMatcher titleMatcher(title);
        titleMatcher.SetStart();
        if (!titleMatcher.IsEmptyTitle()) {
            for (int wid = 0; wid < text.WordsCount(); ++wid) {
                WordVal[wid].IsSuffixOrStopword = text.SentsInfo.WordVal[wid].IsSuffix || text.IsStopword(wid);
                std::pair<bool, bool> match = titleMatcher.MatchStep(text, wid);
                WordAcc[wid + 1].SumDiscountedTitleMatches = WordAcc[wid].SumDiscountedTitleMatches + (match.first ? 1 : 0);
                WordAcc[wid + 1].SumTitleMatches = WordAcc[wid].SumTitleMatches + (match.second ? 1 : 0);
            }
        }
        //title eqinfo matching
        SentVal.resize(text.SentsInfo.SentencesCount());
        if (title) {
            for (size_t i = 0; i < SentVal.size(); ++i) {
                SentVal[i].TitleSimilarity =
                    GetSimilarity(title->GetEQInfo(), text.GetSentEQInfos(i), true);
            }
        }
    }

    double TTitleMatchInfo::GetTitleLikeness(int i, int j) const {
        static const int FULL_COVER_THRESH = 8;
        static const int LEADING_COVER_THRESH = 5;
        static const int LEADING_WORDS = 7;

        int wordPos[LEADING_WORDS];
        int nWordsFound = 0;
        memset(wordPos, 0, sizeof(wordPos));

        if (i > j) {
            return .0;
        }

        int k = i;
        while (k <= j) {
            if (!WordVal[k].IsSuffixOrStopword) {
                wordPos[nWordsFound++] = k;
                if (nWordsFound >= LEADING_WORDS)
                    break;
            }
            ++k;
        }

        if (nWordsFound < 3)
            return .0;

        i = wordPos[2];
        int fullSnipCover = GetRawTitleWordCover(i, j);
        if (fullSnipCover >= FULL_COVER_THRESH) {
            return 1.0;
        }
        if (nWordsFound >= LEADING_WORDS) {
            int end = wordPos[LEADING_WORDS - 1];
            int leadCover = GetRawTitleWordCover(i, end);
            if (leadCover >= LEADING_COVER_THRESH)
                return 1.0;
        } else {
            return fullSnipCover > 0 ? 1.0 : 0.0;
        }
        return .0;
    }

    double TTitleMatchInfo::GetTitleSimilarity(int i) const {
        return SentVal[i].TitleSimilarity;
    }
}
