#pragma once

#include "wordspair.h"
#include "wordhomonymnum.h"
#include "gramhelper2.h"
#include <util/generic/string.h>
#include <util/string/util.h>


class TFIOOccurenceInText;


struct TFullFIO //: public SFIOSimple
{
    TFullFIO() ;
    TFullFIO(const TFullFIO&) = default;
    TFullFIO& operator=(const TFullFIO&) = default;

    EFIOType GetType() const;
    TString Surname;
    TString Name;
    TString Patronymic;
    bool  FoundSurname;
    bool  FirstNameFromMorph;
    bool  MultiWordFio;

    void Reset();
    bool Gleiche(const TFullFIO& fio) const;

    //приписывает граммемы (m_commonMuscGrammems и m_commonFemGrammems) TFIOOccurenceInText
    //исходя из имени и bRurnameLemma (только для несловарных фамилий)
    //void InitGrammems(bool bRurnameLemma, TFIOOccurenceInText& wp);

    bool HasInitials() const;
    bool HasInitialName() const;
    bool HasInitialPatronymic() const;
    bool IsEmpty() const;
    bool operator<(const TFullFIO& FIO) const;
    bool operator==(const TFullFIO& FIO) const;
    bool order_by_f(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_i(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_fi(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_fio(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_fioIn(const TFullFIO& FIO2, bool bUseInitials = true) const;
    bool order_by_fiInoIn(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_io(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_o(const TFullFIO& cluster2, bool bUseInitials = true) const;
    bool order_by_oIn(const TFullFIO& cluster2) const;
    bool order_by_iIn(const TFullFIO& FIO2) const;
    bool order_by_fiIn(const TFullFIO& FIO2, bool bUseInitials = true) const;

    bool default_order(const TFullFIO& cluster2) const;

    bool equal_by_f(const TFullFIO& FIO2, bool bUseInitials = true) const;
    bool equal_by_i(const TFullFIO& FIO2, bool bUseInitials = true) const;
    bool equal_by_o(const TFullFIO& FIO2, bool bUseInitials = true) const;
    TString ToString() const;
    void ToLower();

    TGramBitSet Genders;
    bool  InitialPatronymic;
    bool  InitialName;
    bool  SurnameLemma;
    bool  FoundPatronymic;
    int LocalSuranamePardigmID;
};


class TFIOOccurence : public TWordsPair
{
public:
    TWordHomonymNum NameMembers[NameTypeCount];

    TFIOOccurence()
    {
        SimilarOccurencesCount = 0;
    }

    TFIOOccurence(const TWordsPair& w_p)
    {
        SetPair(w_p);
        SimilarOccurencesCount = 0;
    }

    void SetRealPair()
    {
        int w1 = -1;
        int w2 = -1;
        for(int i = 0 ; i < NameTypeCount ; i++)
        {
            if( NameMembers[i].WordNum != -1 )
            {
                if( w1 == -1 || w1 > NameMembers[i].WordNum)
                    w1 = NameMembers[i].WordNum;
                if( w2 == -1 || w2 < NameMembers[i].WordNum)
                    w2 = NameMembers[i].WordNum;
            }
        }
        SetPair(w1,w2);
    }


    TFIOOccurence(int i)
    {
        SetPair(i,i);
        SimilarOccurencesCount = 0;
    }

    const TGramBitSet& GetGrammems() const   {return Grammems;  }
    void PutGrammems(const TGramBitSet& grammems)    { Grammems = grammems;  }

    int SimilarOccurencesCount;    //количество других вхождений ФИО, которые смогли слиться с этим
                                    //(нужно для выбора варианта у одной цепочки ФИО ("Евгения Петрова"))
    TGramBitSet Grammems;

};

class TFioWordSequence : public TFIOOccurence
{
public:
    TFioWordSequence(const TFullFIO& fio) : Fio(fio)
    {

    }

    TFioWordSequence()
    {
    }

    TFullFIO& GetFio()
    {
        return Fio;
    }

    const TFullFIO& GetFio() const
    {
        return Fio;
    }

    virtual TString GetLemma() const;

    TFullFIO Fio;
};

class IFullTextForFioExtractor
{
public:
    virtual bool HasCommonLemmas(int sent1, int word1, int sent2, int word2) const = 0;
    virtual TPosting GetPosting(int s, int w) const = 0;
    virtual TString GetLemma(int s, int w, int h) const = 0;
    virtual ~IFullTextForFioExtractor() { }
};
