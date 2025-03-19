#pragma once

#include <util/generic/yexception.h>

class TWordsPair
{
public:
    TWordsPair();
    TWordsPair(int i)
    {
        Word1 = i;
        Word2 = i;
    }
    TWordsPair(int w1, int w2)
    {
        Word1 = w1;
        Word2 = w2;
    }
    virtual ~TWordsPair();

//creation and changes
    void SetPair(int iFirstWord, int iLastWord);
    void SetPair(const TWordsPair& wp);
    virtual void ChangeFirstWord(int iFirstWord);
    virtual void ChangeLastWord(int iLastWord);
    virtual TWordsPair* CloneAsWordsPair();

//accessors
    int FirstWord() const
    {
        return Word1;
    }
    int LastWord() const
    {
        return Word2;
    }


//primitive operations with pairs
    bool operator< (const TWordsPair& WordsPair) const;
    bool operator== (const TWordsPair& WordsPair) const;
    bool includes(const TWordsPair& WordsPair) const;
    bool contains(int iW) const;
    bool from_left(const TWordsPair& WordsPair) const;
    bool from_right(const TWordsPair& WordsPair) const;
    bool equal_as_pair(const TWordsPair& WordsPair) const;
    virtual void reset()  { Word1 = -1; Word2 = -1; };
    int     size() const;
    bool intersects(const TWordsPair& wp ) const;
    virtual bool is_vaid() const;

protected:
    int Word1;
    int Word2;
};

//class SPeriodOrder
//{
//public:
//    bool operator()(const TWordsPair* cl1, const TWordsPair* cl2) const
//    {
//        return *(cl1) < *(cl2);
//    }
//};
//


class TWordsPairInText : public TWordsPair
{
public:
    TWordsPairInText();
    TWordsPairInText(long sentNum, const TWordsPair& wp);

    void PutSentNum(int iS) { SentNum = iS; } ;
    int     GetSentNum() const { return SentNum; };

    bool operator< (const TWordsPairInText& WordsPairInText) const;
    bool operator== (const TWordsPairInText& WordsPairInText) const;

protected:
    long SentNum;
};
