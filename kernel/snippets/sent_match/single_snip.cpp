#include "single_snip.h"
#include "sent_match.h"
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_info/sentword.h>
#include <util/digest/numeric.h>

namespace NSnippets
{
    TSingleSnip::TSingleSnip(const std::pair<int, int>& wordRange, const TSentsMatchInfo& sentsMatchInfo)
        : FirstWord(wordRange.first)
        , LastWord(wordRange.second)
        , SentsMatchInfo(&sentsMatchInfo)
    {
    }

    TSingleSnip::TSingleSnip(const int i, const int j, const TSentsMatchInfo& sentsMatchInfo)
        : FirstWord(i)
        , LastWord(j)
        , SentsMatchInfo(&sentsMatchInfo)
    {
    }

    TSingleSnip::TSingleSnip(const TSentWord& i, const TSentWord& j, const TSentsMatchInfo& sentsMatchInfo)
        : FirstWord(i.ToWordId())
        , LastWord(j.ToWordId())
        , SentsMatchInfo(&sentsMatchInfo)
    {
    }

    TSingleSnip::TSingleSnip(const TSentMultiword& i, const TSentMultiword& j, const TSentsMatchInfo& sentsMatchInfo)
        : FirstWord(i.GetFirst().ToWordId())
        , LastWord(j.GetLast().ToWordId())
        , SentsMatchInfo(&sentsMatchInfo)
    {
    }

    int TSingleSnip::GetFirstWord() const {
        return FirstWord;
    }
    int TSingleSnip::GetLastWord() const {
        return LastWord;
    }
    int TSingleSnip::GetFirstSent() const {
        return SentsMatchInfo->SentsInfo.WordId2SentId(FirstWord);
    }
    int TSingleSnip::GetLastSent() const {
        return SentsMatchInfo->SentsInfo.WordId2SentId(LastWord);
    }
    bool TSingleSnip::BeginsWithSentBreak() const {
        return !SentsMatchInfo->SentsInfo.IsWordIdFirstInSent(FirstWord);
    }
    bool TSingleSnip::EndsWithSentBreak() const {
        return !SentsMatchInfo->SentsInfo.IsWordIdLastInSent(LastWord);
    }
    bool TSingleSnip::HasMatches() const {
        return SentsMatchInfo->MatchesInRange(FirstWord, LastWord) != 0;
    }
    TWtringBuf TSingleSnip::GetTextBuf() const {
        return SentsMatchInfo->SentsInfo.GetTextBuf(FirstWord, LastWord);
    }
    int TSingleSnip::WordsCount() const {
        return LastWord - FirstWord + 1;
    }
    size_t TSingleSnip::GetWordsHash() const
    {
        size_t hash = 0;
        for (int i = FirstWord; i <= LastWord; ++i) {
            hash = CombineHashes(hash, SentsMatchInfo->SentsInfo.WordVal[i].Word.Hash);
        }
        return hash;
    }
    std::pair<int, int> TSingleSnip::GetWordRange() const {
        return {FirstWord, LastWord};
    }
    void TSingleSnip::SetWordRange(int first, int second) {
        FirstWord = first;
        LastWord = second;
    }
    const TSentsMatchInfo* TSingleSnip::GetSentsMatchInfo() const {
        return SentsMatchInfo;
    }
    bool TSingleSnip::GetAllowInnerDots() const {
        return AllowInnerDots;
    }
    void TSingleSnip::SetAllowInnerDots(bool value) {
        AllowInnerDots = value;
    }
    bool TSingleSnip::GetForceDotsAtBegin() const {
        return ForceDotsAtBegin;
    }
    void TSingleSnip::SetForceDotsAtBegin(bool value) {
        ForceDotsAtBegin = value;
    }
    bool TSingleSnip::GetForceDotsAtEnd() const {
        return ForceDotsAtEnd;
    }
    void TSingleSnip::SetForceDotsAtEnd(bool value) {
        ForceDotsAtEnd = value;
    }
    ESnipPassageType TSingleSnip::GetSnipType() const {
        return SnipType;
    }
    void TSingleSnip::SetSnipType(ESnipPassageType value) {
        SnipType = value;
    }
    void TSingleSnip::AddPassageAttr(TStringBuf name, TStringBuf value) {
        if (PassageAttrs.size()) {
            PassageAttrs += '\t';
        }
        PassageAttrs += name;
        PassageAttrs += '\t';
        PassageAttrs += value;
    }
    void TSingleSnip::SetPassageAttrs(const TString& value) {
        PassageAttrs = value;
    }
    const TString& TSingleSnip::GetPassageAttrs() const {
        return PassageAttrs;
    }
}
