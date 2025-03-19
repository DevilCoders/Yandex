#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {

    class TQueryy;
    class TSentsMatchInfo;

    enum EPutSpanMode
    {
        PSM_LEFT = 1 << 0,
        PSM_RIGHT = 1 << 1,
    };

    class TSeenCount
    {
    public:
        int StopUser = 0;
        int NonstopUser = 0;
        int StopWizard = 0;
        int NonstopWizard = 0;
    public:
        void Clear();
        void Add(bool isUserWord, bool isStopWord, int num);
        int GetUserCount() const;
        int GetWizardCount() const;
        int GetNonstopCount() const;
        int GetTotal() const;
        bool operator==(const TSeenCount& that) const;
    };

    class TWordStatData : private TNonCopyable
    {
    public:
        const TQueryy& Query;
        TVector<int> SeenQueryLemma;
        TVector<int> SeenQueryPosition;
        TVector<int> TimesForcedPosSeen;
        int ForcedPosSeenCount;
        int RepeatedLemmaSeen;
        int FreqLemmaSeen;
        int FreqPosSeen;
        TVector<std::pair<int, int>> TimesWordSeen; //не чистится, а только счетчики в value зануляются. Кол-во ключей может быть равно кол-ву слов
        // видимых сниппетовщиком. Считать что-то полным обходом по ней НАСТОЙЧЕГО НЕ РЕКОМЕНДУЕТСЯ
        int RepeatedWordSeen;
        TVector<int> SeenLikePos;
        TVector<int> SeenSynLikePos;
        TVector<int> SeenAlmostUserWordsLikePos;
        double SumPosIdfNorm;
        double SumUserPosIdfNorm;
        double SumAlmostUserWordsPosIdfNorm;
        double SumWizardPosIdfNorm;
        TSeenCount WordSeenCount;
        TSeenCount LemmaSeenCount;

        // without repeatings
        TSeenCount LikeWordSeenCount;
        int SynWordSeenCount;

        int Words;
        int UniqueWords;
        int RepeatedWordsScore;
        double SumMatchPosIdfLog;
        double SumUserMatchPosIdfLog;

    public:
        /**
         * @param wordCount - number of unique words in text
         * If wordCount == 0 then some params not calculated in PutWord:
         * UniqueWords, RepeatedWordSeen, RepeatedWordsScore
         */
        TWordStatData(const TQueryy& query, size_t wordsCount);

        void Clear();

        bool Check(const TWordStatData& a, const TSentsMatchInfo& info, int i, int j, ui8 mode = 0);

        void PutPair(const TSentsMatchInfo& info, int a, int b);
        void UnPutPair(const TSentsMatchInfo& info, int a, int b);
        void PutWord(const TSentsMatchInfo& info, int w, const TSentsMatchInfo* w2h);
        void UnPutWord(const TSentsMatchInfo& info, int w);
        void PutSpan(const TSentsMatchInfo& info, int i, int j, ui8 mode = 0, const TSentsMatchInfo* w2h = nullptr);
        void UnPutSpan(const TSentsMatchInfo& info, int i, int j, ui8 mode = 0);

        double LogMatchWordIdfNormSum() const;
        double LogUserMatchWordIdfNormSum() const;
        bool HasAnyMatches() const;
        bool GetSeenAllLikePos() const;
        double GetForcedPosSeenPercent() const;
        double GetUserBM25() const;
    };

}
