#pragma once

#include "sent_info.h"

namespace NSnippets
{
    // stores words numbers in sentences and moves over them
    class TSentWord
    {
    private:
        const TSentsInfo* Info;
        int Sent;
        int WordOfs;
    public:
        TSentWord(const TSentsInfo* info, int sent, int wordOfs)
            : Info(info)
            , Sent(sent)
            , WordOfs(wordOfs)
        {
        }
        bool operator<(const TSentWord& b) const {
            if (Sent != b.Sent)
                return Sent < b.Sent;
            return WordOfs < b.WordOfs;
        }
        bool operator<=(const TSentWord& b) const {
            return !(b < *this);
        }
        bool operator>(const TSentWord& b) const {
            return b < *this;
        }
        bool operator>=(const TSentWord& b) const {
            return !(*this < b);
        }
        bool operator==(const TSentWord& b) const {
            return Sent == b.Sent && WordOfs == b.WordOfs;
        }
        bool operator!=(const TSentWord& b) const {
            return !(*this == b);
        }
        TSentWord& operator--() {
            if (WordOfs-- == 0) {
                --Sent;
                WordOfs = Info->GetSentLengthInWords(Sent) - 1;
            }
            return *this;
        }
        TSentWord& operator++() {
            if (++WordOfs == Info->GetSentLengthInWords(Sent)) {
                ++Sent;
                WordOfs = 0;
            }
            return *this;
        }
        const TSentsInfo* GetInfo() const {
            return Info;
        }
        int ToWordId() const {
            return WordOfs + Info->FirstWordIdInSent(Sent);
        }
        int FirstWordId() const {
            return ToWordId();
        }
        TSentWord FirstSentWord() const {
            return *this;
        }
        int LastWordId() const {
            return ToWordId();
        }
        TSentWord LastSentWord() const {
            return *this;
        }
        int GetSent() const {
            return Sent;
        }
        int GetWordOfs() const {
            return WordOfs;
        }
        TSentWord Prev() const {
            return --TSentWord(*this);
        }
        TSentWord Next() const {
            return ++TSentWord(*this);
        }
        TSentWord MultiwordFirst() const {
            TSentWord res = *this;
            if (res.Sent >= Info->SentencesCount())
                return res;
            while (res.WordOfs && Info->WordVal[res.ToWordId()].IsSuffix)
                --res;
            return res;
        }
        TSentWord MultiwordLast() const {
            TSentWord res = *this;
            if (res.Sent >= Info->SentencesCount())
                return res;
            TSentWord nxt = res.Next();
            while (nxt.Sent == res.Sent && Info->WordVal[nxt.ToWordId()].IsSuffix) {
                res = nxt;
                ++nxt;
            }
            return res;
        }
        TSentWord FirstInSameSent() const {
            return TSentWord(Info, Sent, 0);
        }
        bool IsFirstInSent() const {
            return WordOfs == 0;
        }
        TSentWord LastInSameSent() const {
            return TSentWord(Info, Sent, Info->GetSentLengthInWords(Sent) - 1);
        }
        bool IsLastInSent() const {
            return WordOfs == Info->GetSentLengthInWords(Sent) - 1;
        }
        bool IsFirst() const {
            return Sent == 0 && WordOfs == 0;
        }
        bool IsLast() const {
            return Sent == Info->SentencesCount() - 1 && IsLastInSent();
        }
    };

    // stores begin and end of fragment in text and shifts over them forward/backward
    class TSentMultiword
    {
    private:
        TSentWord First, Last;
    public:
        TSentMultiword(const TSentWord& some)
            : First(some.MultiwordFirst())
            , Last(some.MultiwordLast())
        {
        }
        int FirstWordId() const {
            return First.FirstWordId();
        }
        TSentWord FirstSentWord() const {
            return First;
        }
        int LastWordId() const {
            return Last.LastWordId();
        }
        TSentWord LastSentWord() const {
            return Last;
        }
        int GetSent() const {
            return First.GetSent();
        }
        const TSentsInfo* GetInfo() const {
            return First.GetInfo();
        }
        TSentWord GetFirst() const {
            return First;
        }
        TSentWord GetLast() const {
            return Last;
        }
        TSentMultiword& operator++() {
            First = Last.Next();
            Last = First.MultiwordLast();
            return *this;
        }
        TSentMultiword& operator--() {
            Last = First.Prev();
            First = Last.MultiwordFirst();
            return *this;
        }
        TSentMultiword Prev() const {
            return --TSentMultiword(*this);
        }
        TSentMultiword Next() const {
            return ++TSentMultiword(*this);
        }
        TSentMultiword FirstInSameSent() const {
            return TSentMultiword(First.FirstInSameSent());
        }
        bool IsFirstInSent() const {
            return First.IsFirstInSent();
        }
        TSentMultiword LastInSameSent() const {
            return TSentMultiword(First.LastInSameSent());
        }
        bool IsFirst() const {
            return First.IsFirst();
        }
        bool IsLast() const {
            return Last.IsLast();
        }
        bool IsLastInSent() const {
            return Last.IsLastInSent();
        }
        bool operator<(const TSentMultiword& other) const {
            return First < other.First;
        }
        bool operator>(const TSentMultiword& other) const {
            return First > other.First;
        }
        bool operator>=(const TSentMultiword& other) const {
            return !(*this < other);
        }
        bool operator<=(const TSentMultiword& other) const {
            return !(*this > other);
        }
        bool operator==(const TSentMultiword& other) const {
            return First == other.First;
        }
        bool operator!=(const TSentMultiword& other) const {
            return First != other.First;
        }
        TWtringBuf GetWordBuf() const {
            return GetInfo()->GetWordSpanBuf(FirstWordId(), LastWordId());
        }
    };

}
