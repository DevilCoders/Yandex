#include "wordstat_data.h"
#include <kernel/snippets/sent_match/sent_match.h>

#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <util/charset/unidata.h>
#include <util/generic/ymath.h>
#include <util/system/yassert.h>

namespace NSnippets {

    template <class T>
    static void CheckVectors(const TVector<T>& a, const TVector<T>& b)
    {
        Y_ASSERT(a.size() == b.size());
        for(size_t i =0; i < a.size(); ++i)
            Y_ASSERT(a[i] == b[i]);
    }

    template <class T>
    static void NoStrictCheck(const T& a, const T& b)
    {
        Y_ASSERT(Abs(a - b) < 1e-9f);
    }

    void TSeenCount::Clear() {
        StopUser = 0;
        NonstopUser = 0;
        StopWizard = 0;
        NonstopWizard = 0;
    }
    void TSeenCount::Add(bool isUserWord, bool isStopWord, int num) {
        if (isUserWord) {
            if (isStopWord) {
                StopUser += num;
            } else {
                NonstopUser += num;
            }
        } else {
            if (isStopWord) {
                StopWizard += num;
            } else {
                NonstopWizard += num;
            }
        }
    }
    int TSeenCount::GetUserCount() const {
        return StopUser + NonstopUser;
    }
    int TSeenCount::GetWizardCount() const {
        return StopWizard + NonstopWizard;
    }
    int TSeenCount::GetNonstopCount() const {
        return NonstopUser + NonstopWizard;
    }
    int TSeenCount::GetTotal() const {
        return StopUser + NonstopUser + StopWizard + NonstopWizard;
    }
    bool TSeenCount::operator==(const TSeenCount& that) const {
        return StopUser == that.StopUser && NonstopUser == that.NonstopUser &&
            StopWizard == that.StopWizard && NonstopWizard == that.NonstopWizard;
    }

    TWordStatData::TWordStatData(const TQueryy& query, size_t wordsCount)
      : Query(query)
      , SeenQueryLemma(size_t(query.IdsCount()), 0)
      , SeenQueryPosition(size_t(query.PosCount()), 0)
      , TimesForcedPosSeen(size_t(query.PosCount()), 0)
      , ForcedPosSeenCount(0)
      , RepeatedLemmaSeen(0)
      , FreqLemmaSeen(0)
      , FreqPosSeen(0)
      , TimesWordSeen(wordsCount, std::make_pair(0, 0))
      , RepeatedWordSeen(0)
      , SeenLikePos(size_t(query.PosCount()), 0)
      , SeenSynLikePos(size_t(query.PosCount()), 0)
      , SeenAlmostUserWordsLikePos(size_t(query.PosCount()), 0)
      , SumPosIdfNorm(0.0)
      , SumUserPosIdfNorm(0.0)
      , SumAlmostUserWordsPosIdfNorm(0.0)
      , SumWizardPosIdfNorm(0.0)
      , SynWordSeenCount(0)
      , Words(0)
      , UniqueWords(0)
      , RepeatedWordsScore(0)
      , SumMatchPosIdfLog(0.0)
      , SumUserMatchPosIdfLog(0.0)
    {
    }

    void TWordStatData::Clear()
    {
        Fill(SeenQueryLemma.begin(), SeenQueryLemma.end(), 0);
        Fill(SeenQueryPosition.begin(), SeenQueryPosition.end(), 0);
        Fill(TimesForcedPosSeen.begin(), TimesForcedPosSeen.end(), 0);
        Fill(SeenLikePos.begin(), SeenLikePos.end(), 0);
        Fill(SeenSynLikePos.begin(), SeenSynLikePos.end(), 0);
        Fill(SeenAlmostUserWordsLikePos.begin(), SeenAlmostUserWordsLikePos.end(), 0);
        ForcedPosSeenCount = 0;
        RepeatedLemmaSeen = 0;
        FreqLemmaSeen = 0;
        FreqPosSeen = 0;
        RepeatedWordSeen = 0;
        SumPosIdfNorm = 0.0;
        SumUserPosIdfNorm = 0.0;
        SumAlmostUserWordsPosIdfNorm = 0.0;
        SumWizardPosIdfNorm = 0.0;
        WordSeenCount.Clear();
        LemmaSeenCount.Clear();
        LikeWordSeenCount.Clear();
        SynWordSeenCount = 0;
        Words = 0;
        UniqueWords = 0;
        RepeatedWordsScore = 0;
        SumMatchPosIdfLog = 0.0;
        SumUserMatchPosIdfLog = 0.0;

        for (auto& t : TimesWordSeen) {
            t.first = 0;
            t.second = 0;
        }
    }

    bool TWordStatData::Check(const TWordStatData& a, const TSentsMatchInfo& info, int i, int j, ui8 mode)
    {
        PutSpan(info, i, j, mode);

        Y_ASSERT(ForcedPosSeenCount == a.ForcedPosSeenCount);
        Y_ASSERT(RepeatedLemmaSeen == a.RepeatedLemmaSeen);
        Y_ASSERT(FreqLemmaSeen == a.FreqLemmaSeen);
        Y_ASSERT(FreqPosSeen == a.FreqPosSeen);
        Y_ASSERT(RepeatedWordSeen == a.RepeatedWordSeen);
        Y_ASSERT(WordSeenCount == a.WordSeenCount);
        Y_ASSERT(LemmaSeenCount == a.LemmaSeenCount);
        Y_ASSERT(LikeWordSeenCount == a.LikeWordSeenCount);
        Y_ASSERT(SynWordSeenCount == a.SynWordSeenCount);

        Y_ASSERT(Words == a.Words);
        Y_ASSERT(UniqueWords == a.UniqueWords);
        Y_ASSERT(RepeatedWordsScore == a.RepeatedWordsScore);

        NoStrictCheck(SumMatchPosIdfLog, a.SumMatchPosIdfLog);
        NoStrictCheck(SumUserMatchPosIdfLog, a.SumUserMatchPosIdfLog);
        NoStrictCheck(SumPosIdfNorm, a.SumPosIdfNorm);
        NoStrictCheck(SumUserPosIdfNorm, a.SumUserPosIdfNorm);
        NoStrictCheck(SumAlmostUserWordsPosIdfNorm, a.SumAlmostUserWordsPosIdfNorm);
        NoStrictCheck(SumWizardPosIdfNorm, a.SumWizardPosIdfNorm);

        CheckVectors(SeenQueryLemma, a.SeenQueryLemma);
        CheckVectors(SeenQueryPosition, a.SeenQueryPosition);
        CheckVectors(TimesForcedPosSeen, a.TimesForcedPosSeen);
        CheckVectors(SeenLikePos, a.SeenLikePos);
        CheckVectors(SeenSynLikePos, a.SeenSynLikePos);
        CheckVectors(SeenAlmostUserWordsLikePos, a.SeenAlmostUserWordsLikePos);

        CheckVectors(TimesWordSeen, a.TimesWordSeen);

        return true;
    }

    void TWordStatData::PutPair(const TSentsMatchInfo& info, int a, int b)
    {
        if (Query.MaxForcedNeighborPosCount == 0) {
            return;
        }
        const TVector<int>& la = info.GetNotExactMatchedLemmaIds(a);
        const TVector<int>& lb = info.GetNotExactMatchedLemmaIds(b);
        for (int i : la) {
            for (int j : lb) {
                std::pair<int, int> key = {Min(i, j), Max(i, j)};
                const int* forsedPos = Query.LemmaPairToForcedNeighborPos.FindPtr(key);
                if (forsedPos) {
                    if (TimesForcedPosSeen[*forsedPos]++ == 0) {
                        ++ForcedPosSeenCount;
                    }
                }
            }
        }
    }

    void TWordStatData::UnPutPair(const TSentsMatchInfo& info, int a, int b)
    {
        if (Query.MaxForcedNeighborPosCount == 0) {
            return;
        }
        const TVector<int>& la = info.GetNotExactMatchedLemmaIds(a);
        const TVector<int>& lb = info.GetNotExactMatchedLemmaIds(b);
        for (int i : la) {
            for (int j : lb) {
                std::pair<int, int> key = {Min(i, j), Max(i, j)};
                const int* forsedPos = Query.LemmaPairToForcedNeighborPos.FindPtr(key);
                if (forsedPos) {
                    if (--TimesForcedPosSeen[*forsedPos] == 0) {
                        --ForcedPosSeenCount;
                    }
                }
            }
        }
    }

    void TWordStatData::PutWord(const TSentsMatchInfo& info, int w, const TSentsMatchInfo* w2h)
    {
        ++Words;

        for (int pos : info.GetExactMatchedPositions(w)) {
            if (SeenQueryPosition[pos]++ == 0) {
                const auto& queryPos = Query.Positions[pos];
                WordSeenCount.Add(queryPos.IsUserWord, queryPos.IsStopWord, 1);
            }
            if (SeenQueryPosition[pos] == 2)
                ++FreqPosSeen;
        }

        for (int lemmaId : info.GetNotExactMatchedLemmaIds(w)) {
            if (SeenQueryLemma[lemmaId]++ == 0) {
                const auto& queryLemma = Query.Lemmas[lemmaId];
                LemmaSeenCount.Add(queryLemma.LemmaIsUserWord, queryLemma.LemmaIsPureStopWord, 1);
            }
            for (int pos : Query.Id2Poss[lemmaId]) {
                if (SeenLikePos[pos]++ == 0) {
                    const auto& queryPos = Query.Positions[pos];
                    SumPosIdfNorm += queryPos.IdfNorm;
                    SumUserPosIdfNorm += queryPos.UserIdfNorm;
                    SumWizardPosIdfNorm += queryPos.WizardIdfNorm;
                    if (!queryPos.IsStopWord) {
                        SumMatchPosIdfLog += queryPos.IdfLog;
                        SumUserMatchPosIdfLog += queryPos.UserIdfLog;
                    }
                    LikeWordSeenCount.Add(queryPos.IsUserWord, queryPos.IsStopWord, 1);
                }
            }
            if (SeenQueryLemma[lemmaId] == 2)
                ++FreqLemmaSeen;
            if (SeenQueryLemma[lemmaId] > 3)
                ++RepeatedLemmaSeen;
        }

        for (int synLemmaId : info.GetSynMatchedLemmaIds(w)) {
            for (int pos : Query.Id2Poss[synLemmaId]) {
                if (SeenSynLikePos[pos]++ == 0) {
                    ++SynWordSeenCount;
                }
            }
        }

        for (int almostUserWordsLemmaId : info.GetAlmostUserWordsMatchedLemmaIds(w)) {
            for (int pos : Query.Id2Poss[almostUserWordsLemmaId]) {
                if (SeenAlmostUserWordsLikePos[pos]++ == 0) {
                    SumAlmostUserWordsPosIdfNorm += Query.Positions[pos].AlmostUserWordsIdfNorm;
                }
            }
        }

        //см. коментарий к объявлению TimesWordSeen => UniqueWords считаем прямо здесь и в явном виде
        if (!TimesWordSeen.empty()) {
            const TSentsInfo::TWordVal& wordVal = info.SentsInfo.WordVal[w];
            int wNum = wordVal.Word.N;
            if (w2h) {
                const size_t* pNum = w2h->SentsInfo.W2H.FindPtr(wordVal.Word.Hash);
                wNum = pNum ? *pNum : -1;
            }
            if (wNum >= 0) {
                std::pair<int, int>& p = TimesWordSeen[wNum];
                if (p.first++ == 0) {
                    ++UniqueWords;
                }
                if (!info.GetNotExactMatchedLemmaIds(w) && !info.IsStopword(w)) {
                    if (p.second++ > 0) {
                        ++RepeatedWordSeen;
                    }
                }
                // RepeatedWordsScore - это разность количества повторяющихся слов и количества уникальных слов
                TWtringBuf word = info.SentsInfo.GetWordBuf(w);
                if (!info.IsStopword(w) && word && !IsDigit(word[0])) {
                    if (p.first == 1) {
                        --RepeatedWordsScore;
                    } else if (p.first >= 2) {
                        ++RepeatedWordsScore;
                    }
                    if (p.first == 2) {
                        // предыдущее вхождение этого слова было посчитано как уникальное, нужно исправить это
                        RepeatedWordsScore += 2;
                    }
                }
            }
        }
    }

    void TWordStatData::UnPutWord(const TSentsMatchInfo& info, int w)
    {
        Y_ASSERT(w >= 0);
        --Words;

        for (int pos : info.GetExactMatchedPositions(w)) {
            if (SeenQueryPosition[pos] == 2)
                --FreqPosSeen;
            if (--SeenQueryPosition[pos] == 0) {
                const auto& queryPos = Query.Positions[pos];
                WordSeenCount.Add(queryPos.IsUserWord, queryPos.IsStopWord, -1);
            }
        }

        for (int lemmaId : info.GetNotExactMatchedLemmaIds(w)) {
            if (SeenQueryLemma[lemmaId] == 2)
                --FreqLemmaSeen;
            if (SeenQueryLemma[lemmaId] > 3)
                --RepeatedLemmaSeen;
            if (--SeenQueryLemma[lemmaId] == 0) {
                const auto& queryLemma = Query.Lemmas[lemmaId];
                LemmaSeenCount.Add(queryLemma.LemmaIsUserWord, queryLemma.LemmaIsPureStopWord, -1);
            }
            for (int pos : Query.Id2Poss[lemmaId]) {
                if (--SeenLikePos[pos] == 0) {
                    const auto& queryPos = Query.Positions[pos];
                    SumPosIdfNorm -= queryPos.IdfNorm;
                    SumUserPosIdfNorm -= queryPos.UserIdfNorm;
                    SumWizardPosIdfNorm -= queryPos.WizardIdfNorm;
                    if (!queryPos.IsStopWord) {
                        SumMatchPosIdfLog -= queryPos.IdfLog;
                        SumUserMatchPosIdfLog -= queryPos.UserIdfLog;
                    }
                    LikeWordSeenCount.Add(queryPos.IsUserWord, queryPos.IsStopWord, -1);
                }
            }
        }

        for (int synLemmaId : info.GetSynMatchedLemmaIds(w)) {
            for (int pos : Query.Id2Poss[synLemmaId]) {
                if (--SeenSynLikePos[pos] == 0) {
                    --SynWordSeenCount;
                }
            }
        }

        for (int almostUserWordsLemmaId : info.GetAlmostUserWordsMatchedLemmaIds(w)) {
            for (int pos : Query.Id2Poss[almostUserWordsLemmaId]) {
                if (--SeenAlmostUserWordsLikePos[pos] == 0) {
                    SumAlmostUserWordsPosIdfNorm -= Query.Positions[pos].AlmostUserWordsIdfNorm;
                }
            }
        }

        if (!TimesWordSeen.empty()) {
            const TSentsInfo::TWordVal& wordVal = info.SentsInfo.WordVal[w];
            TWtringBuf word = info.SentsInfo.GetWordBuf(w);
            std::pair<int, int>& p = TimesWordSeen[wordVal.Word.N];
            if (!info.IsStopword(w) && word && !IsDigit(word[0])) {
                if (p.first == 1) {
                    ++RepeatedWordsScore;
                } else if (p.first >= 2) {
                    --RepeatedWordsScore;
                }
                if (p.first == 2) {
                    RepeatedWordsScore -= 2;
                }
            }
            if (--p.first == 0) {
                --UniqueWords;
            }
            if (!info.GetNotExactMatchedLemmaIds(w) && !info.IsStopword(w)) {
                if (--p.second > 0) {
                    --RepeatedWordSeen;
                }
            }
        }
    }

    void TWordStatData::PutSpan(const TSentsMatchInfo& info, int i, int j, ui8 mode, const TSentsMatchInfo* w2h)
    {
        for (int t = i; t <= j; ++t)
            PutWord(info, t, w2h);
        for (int t = i + (mode & PSM_LEFT ? 0 : 1); t <= j + (mode & PSM_RIGHT ? 1 : 0); ++t)
            PutPair(info, t - 1, t);
    }

    void TWordStatData::UnPutSpan(const TSentsMatchInfo& info, int i, int j, ui8 mode)
    {
        for (int t = i; t <= j; ++t)
            UnPutWord(info, t);
        for (int t = i + (mode & PSM_LEFT ? 0 : 1); t <= j + (mode & PSM_RIGHT ? 1 : 0); ++t)
            UnPutPair(info, t - 1, t);
    }

    double TWordStatData::LogMatchWordIdfNormSum() const
    {
        return Query.SumIdfLog > 0 ? SumMatchPosIdfLog / Query.SumIdfLog : 0;
    }

    double TWordStatData::LogUserMatchWordIdfNormSum() const
    {
        return Query.SumUserIdfLog > 0 ? SumUserMatchPosIdfLog / Query.SumUserIdfLog : 0;
    }

    bool TWordStatData::HasAnyMatches() const
    {
        return LemmaSeenCount.GetTotal() + WordSeenCount.GetTotal() > 0;
    }

    bool TWordStatData::GetSeenAllLikePos() const
    {
        return Query.PosCount() == LikeWordSeenCount.GetTotal();
    }

    double TWordStatData::GetForcedPosSeenPercent() const
    {
        return Query.MaxForcedNeighborPosCount == 0 ? 0 : double(ForcedPosSeenCount) / Query.MaxForcedNeighborPosCount;
    }

    double TWordStatData::GetUserBM25() const
    {
        const size_t n = SeenQueryPosition.size();
        const double k1 = 2.0;
        const double docLen = Words;
        double res = 0;
        if (!docLen) {
            return res;
        }
        for (size_t i = 0; i < n; ++i) {
            const double tf = SeenQueryPosition[i] * 1.0 / docLen;
            res += Query.Positions[i].UserIdfNorm * (tf * (k1 + 1)) / (tf + k1);
        }
        return res;
    }

}
