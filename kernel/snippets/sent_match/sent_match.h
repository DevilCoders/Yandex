#pragma once

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>

namespace NSnippets
{

    enum ESubTGrammar {
        STG_UNKNOWN,
        STG_NONE,
        STG_VERB,
        STG_ADJECTIVE,
        STG_NOUN,
        STG_BAD_POS // POS stands for Part of Speach
    };

    class TConfig;
    class TQueryy;
    class TSentsInfo;
    class TSentWord;
    class TSentMultiword;
    class TEQInfo;
    class TPixelLengthCalculator;
    class TBoldSpan;

    class TSentsMatchInfo : private TNonCopyable
    {
    public:
        typedef TVector<int> TPositionsInQuery;
        const TSentsInfo& SentsInfo;
        const TQueryy& Query;
        const TConfig& Cfg;
        const ELanguage DocLangId;
        const TSentsMatchInfo* TitleSentsMatchInfo;

    private:
        struct TData;

        THolder<TData> Data;

    public:
        TSentsMatchInfo(const TSentsInfo& sentsInfo, const TQueryy& query, const TConfig& cfg, ELanguage docLangId = LANG_UNK, const TSentsMatchInfo* titleSentsMatchInfo = nullptr);
        ~TSentsMatchInfo();

    private:
        void InitWords();
        void InitSents();

        bool LeaveStopword(int i, int pos) const;
        bool LeaveStopword(int i) const;
        void KillLonelyStopWords();
        void FillDates();
        void FillTelephones();
        void FillShortWords();
        void FillMatchSumms();
        void FillPornWeight(int wid, const TVector<TUtf16String>& normalizedWords);
        void FillSegments(int wid);
        void FillRegionMatch(int wid, const TVector<TUtf16String>& normalizedWords);
        void FillReadability(int wid);
        void FillAnswerWeight(int wid, const TVector<TUtf16String>& normalizedWords);

        ESubTGrammar GetSubGrammar(int wordId) const;
        bool IsSentRepeatsTitle(int firstWordId, int lastWordId, const THashSet<TWtringBuf>& titleLowerWords, const TVector<TWtringBuf>& titleLowerWordsOrdered) const;

    public:
        const TPixelLengthCalculator& GetPixelLengthCalculator() const;
        bool IsWordAfterTerminal(int i) const;
        ESubTGrammar GetWordSubGrammar(int i) const;
        const TPositionsInQuery& GetExactMatchedPositions(int i) const;
        const TPositionsInQuery& GetNotExactMatchedLemmaIds(int i) const;
        const TPositionsInQuery& GetSynMatchedLemmaIds(int i) const;
        const TPositionsInQuery& GetAlmostUserWordsMatchedLemmaIds(int i) const;
        int WordsCount() const;
        TLangMask GetWordLangs(int i) const;
        bool IsStopword(int i) const;
        const TUtf16String& GetLowerWord(int i) const;
        bool IsExactMatch(int i) const;
        bool IsNotExactMatch(int i) const;
        bool IsMatch(int i) const;
        bool IsMatch(const TSentWord& i) const;
        bool IsMatch(const TSentMultiword& i) const;
        bool IsExactUserPhone(int i) const;
        bool IsPureStopWordMatch(int i) const;
        int MatchesInRange(int i) const;
        int MatchesInRange(int i, int j) const;
        int MatchesInRange(const TSentWord& i, const TSentWord& j) const;
        int MatchesInRange(const TSentMultiword& i, const TSentMultiword& j) const;
        int ExactMatchesInRange(int i, int j) const;
        int NotExactMatchesInRange(int i, int j) const;
        int ShortsInRange(int i, int j) const;
        int StrangeGapsInRange(int i, int j) const;
        int TrashInGapsInRange(int i, int j) const;
        int PunctGapsInRange(int i, int j) const;
        int SlashesInGapsInRange(int i, int j) const;
        int VertsInGapsInRange(int i, int j) const;
        int PunctReadInGapInRange(int i, int j) const;
        int PunctBalInGapInRange(int i, int j) const;
        int PunctBalInGapInsideRange(int i, int j) const;
        int TrashAsciiInGapInRange(int i, int j) const;
        int TrashUTFInGapInRange(int i, int j) const;
        int CapsInRange(int i, int j) const;
        int LangMatchsInRange(int i, int j) const;
        int AlphasInRange(int i, int j) const;
        int CyrAlphasInRange(int i, int j) const;
        int DigitsInrange(int i, int j) const;
        //! Calculates telephone zones count in specified word range
        //! Zone is counted only if both of it bounds belongs to the range
        int TelephonesInRange(int i, int j) const;
        //! Calculates date zones count in specified word range
        //! Zone is counted only if both of it bounds belongs to the range
        int DatesInRange(int i, int j) const;
        int GetLongestMatchChainInSpanRange(int i, int j) const;
        double GetFooterWords(int i, int j) const;
        double GetContentWords(int i, int j) const;
        double GetMainContentWords(int i, int j) const;
        double GetSegmentWeightSums(int i, int j) const;
        double GetHeaderWords(int i, int j) const;
        double GetMainHeaderWords(int i, int j) const;
        double GetMenuWords(int i, int j) const;
        double GetReferatWords(int i, int j) const;
        double GetAuxWords(int i, int j) const;
        double GetLinksWords(int i, int j) const;
        double GetAnswerWeight(int i, int j) const;
        int GetKandziWords(int i, int j) const;
        bool RegionMatchInRange(int i, int j) const;
        int GetUnmatchedByLinkPornCountInRange(int i, int j) const;

    public:
        int GetFirstMatchSentId() const;
        int GetLastMatchSentId() const;
        const TEQInfo& GetSentEQInfos(int i) const;
        int GetSentQuality(int i) const;
        bool SentHasMatches(int i) const;
        bool SentLooksLikeDefinition(int i) const;
        bool SentRepeatsTitle(int i) const;
        bool SentsHasMatches() const;
        int SumQualityInSentRange(int si, int sj) const;
        int NavlikeInSentRange(int si, int sj) const;
        double SumAnchorPercentInSentRange(int startSent, int endSent) const;
        int GetInfoBonusSentsInRange(int i, int j) const;
        int GetInfoBonusMatchSentsInRange(int i, int j) const;
        int GetLongestMatchChainInRange(int i, int j) const;
        int AdsSentsInRange(int i, int j) const;
        int HeaderSentsInRange(int i, int j) const;
        int PollSentsInRange(int i, int j) const;
        int MenuSentsInRange(int i, int j) const;
        int FooterSentsInRange(int i, int j) const;
        int TextAreaSentsInRange(int i, int j) const;
        TVector<TBoldSpan> GetMatchedWords() const;
    };
}
