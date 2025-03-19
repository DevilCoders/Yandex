#pragma once

#include "sent_match.h"

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <utility>

namespace NSnippets
{
    class TSentWord;
    class TSentMultiword;

    enum ESnipPassageType {
        SPT_COMMON = 0,
        SPT_TABLE
    };

    class TSingleSnip
    {
    private:
        int FirstWord = 0;
        int LastWord = 0;
        const TSentsMatchInfo* SentsMatchInfo = nullptr;
        bool AllowInnerDots = false;
        bool ForceDotsAtBegin = false;
        bool ForceDotsAtEnd = false;
        ESnipPassageType SnipType = SPT_COMMON;
        TString PassageAttrs;

    public:
        TSingleSnip(const std::pair<int, int>& wordRange, const TSentsMatchInfo& sentsMatchInfo);
        TSingleSnip(const int i, const int j, const TSentsMatchInfo& sentsMatchInfo);
        TSingleSnip(const TSentWord& i, const TSentWord& j, const TSentsMatchInfo& sentsMatchInfo);
        TSingleSnip(const TSentMultiword& i, const TSentMultiword& j, const TSentsMatchInfo& sentsMatchInfo);
        int GetFirstWord() const;
        int GetLastWord() const;
        int GetFirstSent() const;
        int GetLastSent() const;
        bool BeginsWithSentBreak() const;
        bool EndsWithSentBreak() const;
        bool HasMatches() const;
        TWtringBuf GetTextBuf() const;
        int WordsCount() const;
        size_t GetWordsHash() const;
        std::pair<int, int> GetWordRange() const;
        void SetWordRange(int first, int second);
        const TSentsMatchInfo* GetSentsMatchInfo() const;
        bool GetAllowInnerDots() const;
        void SetAllowInnerDots(bool value);
        bool GetForceDotsAtBegin() const;
        void SetForceDotsAtBegin(bool value);
        bool GetForceDotsAtEnd() const;
        void SetForceDotsAtEnd(bool value);
        ESnipPassageType GetSnipType() const;
        void SetSnipType(ESnipPassageType value);
        void AddPassageAttr(TStringBuf name, TStringBuf value);
        void SetPassageAttrs(const TString& value);
        const TString& GetPassageAttrs() const;
    };
}
