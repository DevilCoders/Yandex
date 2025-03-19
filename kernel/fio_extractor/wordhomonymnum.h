#pragma once

struct TWordHomonymNum
{
    TWordHomonymNum()
    {
        WordNum = -1;
        HomNum = -1;
    };

    virtual ~TWordHomonymNum()
    {
    };


    TWordHomonymNum(int wordNum, int homNum)
    {
        WordNum = wordNum;
        HomNum = homNum;
    };

    virtual bool IsValid() const
    {
        return (WordNum != -1) && (HomNum != -1);
    }

    virtual void Reset()
    {
        WordNum = -1;
        HomNum = -1;
    }


    bool EqualByWord(const TWordHomonymNum& WordHomomonymNum) const
    {
        return (WordHomomonymNum.WordNum == WordNum) ;
    }

    bool IsValidWordNum() const
    {
        return (WordNum != -1);
    }

    bool operator==(const TWordHomonymNum& WordHomomonymNum) const
    {
        return (WordHomomonymNum.HomNum == HomNum) &&
               (WordHomomonymNum.WordNum == WordNum) ;
    }
    int    WordNum;
    int HomNum;
};
