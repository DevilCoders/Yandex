#include "wordspair.h"


TWordsPair::TWordsPair()
{
    Word1 = -1;
    Word2 = -1;
}

TWordsPair::~TWordsPair()
{

}

void TWordsPair::SetPair(const TWordsPair& wp)
{
    if( wp.Word2 < wp.Word1 )
        ythrow yexception() << "Bad Word1-Word2 in TWordsPair.";

    Word1 = wp.Word1;
    Word2 = wp.Word2;

}

void TWordsPair::SetPair(int iFirstWord, int iLastWord)
{
    if( iLastWord < iFirstWord )
        ythrow yexception() << "Bad Word1-Word2 in TWordsPair.";

    Word1 = iFirstWord;
    Word2 = iLastWord;
}

TWordsPair* TWordsPair::CloneAsWordsPair()
{
    /*TWordsPair* pRes = new TWordsPair();
    (*pRes) = this;
    return pRes;*/
    ythrow yexception() << " \"TWordsPair::CloneAsWordsPair\" must be o";
}

bool TWordsPair::is_vaid() const
{
    return (Word1 != -1) && (Word2 != -1);
}

void TWordsPair::ChangeFirstWord(int iFirstWord)
{
    if( Word2 < iFirstWord )
        ythrow yexception() << "Bad Word1-Word2 in TWordsPair.";

    Word1 = iFirstWord;
}

bool TWordsPair::intersects(const TWordsPair& wp ) const
{
    return !(from_left(wp) || wp.from_left(*this));
}


void TWordsPair::ChangeLastWord(int iLastWord)
{
    if( iLastWord < Word1 )
        ythrow yexception() << "Bad Word1-Word2 in TWordsPair.";

    Word2 = iLastWord;
}



int     TWordsPair::size() const
{
    return Word2 - Word1 + 1;
}


bool TWordsPair::contains(int iW) const
{
    return (Word1 <= iW) && (Word2 >= iW);
}

bool TWordsPair::operator< (const TWordsPair& WordsPair) const
{
    return ( (WordsPair.includes(*this) || WordsPair.from_right(*this)) && !(*this == WordsPair));
}

bool TWordsPair::operator== (const TWordsPair& WordsPair) const
{
    return    (Word1 == WordsPair.Word1) &&
            (Word2  == WordsPair.Word2);
}

bool TWordsPair::equal_as_pair(const TWordsPair& WordsPair) const
{
    return (*this) == WordsPair;
}

bool TWordsPair::includes(const TWordsPair& WordsPair) const
{
    return    (Word1 <= WordsPair.Word1) &&
            (Word2  >= WordsPair.Word2);

}

bool TWordsPair::from_left(const TWordsPair& WordsPair) const
{
    return    (Word1 < WordsPair.Word1) &&
            (Word2  < WordsPair.Word1);
}

bool TWordsPair::from_right(const TWordsPair& WordsPair) const
{
    return    (Word1 > WordsPair.Word2) &&
            (Word2  > WordsPair.Word2);
}

TWordsPairInText::TWordsPairInText()
{
    SentNum = -1;
}

TWordsPairInText::TWordsPairInText(long sentNum, const TWordsPair& wp) : TWordsPair(wp)
{
    SentNum = sentNum;
}

bool TWordsPairInText::operator< (const TWordsPairInText& WordsPairInText) const
{
    if( SentNum == WordsPairInText.SentNum )
        return TWordsPair::operator <(WordsPairInText);

    return SentNum < WordsPairInText.SentNum;
}

bool TWordsPairInText::operator== (const TWordsPairInText& WordsPairInText) const
{
    return (SentNum == WordsPairInText.SentNum) && TWordsPair::operator ==(WordsPairInText);
}


