#pragma once

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

#include <functional>

#include "wordspair.h"
#include "fiowordsequence.h"

class TFIOOccurenceInText : public TFIOOccurence
{
public:
    TFIOOccurenceInText() {SentNum = -1;};

    void PutSentNum(int iS) { SentNum = iS; } ;
    int     GetSentNum() const { return SentNum; };

    bool operator< (const TFIOOccurenceInText& WordsPairInText) const
    {
        if(  SentNum < WordsPairInText.SentNum )
            return true;

        if(  SentNum > WordsPairInText.SentNum )
            return false;

        if( TWordsPair::operator <(WordsPairInText) )
            return true;

        if( WordsPairInText.TWordsPair::operator <(*this) )
            return false;

        TGramBitSet g1 = (GetGrammems() & NSpike::AllGenders);// >> (gFeminine - gFirst);
        TGramBitSet g2 = (WordsPairInText.GetGrammems() & NSpike::AllGenders);// >> (gFeminine - gFirst);

        if( g1 < g2 )
            return true;

        if( g2 < g1 )
            return false;

        if( NameMembers[Surname].WordNum < WordsPairInText.NameMembers[Surname].WordNum )
            return true;

        if( NameMembers[Surname].WordNum > WordsPairInText.NameMembers[Surname].WordNum )
            return false;

        return NameMembers[Surname].HomNum < WordsPairInText.NameMembers[Surname].HomNum;
    }

    bool operator== (const TFIOOccurenceInText& WordsPairInText) const
    {
        return (SentNum == WordsPairInText.SentNum) && TWordsPair::operator ==(WordsPairInText);
    }

protected:
    long SentNum;
};


class TNameCluster
{

friend class TFioClusterBuilder;
friend class CTextRus;

public:
    TNameCluster();
    TNameCluster(const TNameCluster& );
    virtual ~TNameCluster();
    void Merge(const TNameCluster& cluster, bool bUniteCoordinate = true);
    void MergePreservingType(const TNameCluster& cluster, bool bUniteCoordinate = true);
    bool Gleiche(const TNameCluster& cluster) const;

    const TFullFIO& GetFullName() const { return FullName; }

    bool operator<(const TNameCluster& Cluster) const;
    bool operator==(const TNameCluster& Cluster) const;
    bool defaul_order(const TNameCluster& cluster2) const;
    bool order_by_f(const TNameCluster& cluster2) const;
    bool order_by_f_pointer(const TNameCluster* cluster2) const;
    bool order_by_i(const TNameCluster& cluster2) const;
    bool order_by_fi(const TNameCluster& cluster2) const;
    bool order_by_fiIn(const TNameCluster& cluster2) const;
    bool order_by_fio(const TNameCluster& cluster2) const;
    bool order_by_io(const TNameCluster& cluster2) const;
    bool order_by_o(const TNameCluster& cluster2) const;
    bool order_by_fiInoIn(const TNameCluster& cluster2) const;
    bool order_by_fioIn(const TNameCluster& cluster2) const;

    TSet<TFIOOccurenceInText>    NameVariants;
    TString ToString() const;

protected:
    TFullFIO    FullName;
    bool ToDel;
    int SurnameIsLemmaCount;
private:
    void MergeSurname(const TNameCluster& cluster, bool preservingType);
    void UniteCoordinate(const TNameCluster& cluster);
};

typedef bool (TNameCluster::*FNameClusterOrder)(const TNameCluster& )const;


class TSetOfFioOccurnces
{
public:
    TSetOfFioOccurnces() {};
    TVector<TFIOOccurenceInText> Occurences;
};

class TFioClusterBuilder
{

    struct SCluster
    {
        EFIOType m_FioTYPE;
        int    m_iCluster;
    };


public:
    TFioClusterBuilder(const IFullTextForFioExtractor* fullText);
    virtual ~TFioClusterBuilder();
    void AddFIO(const TFullFIO& FIO, const TFIOOccurenceInText& group, bool bSuspiciousPatronymic /*= false*/, bool bSurnameLemma /*= false*/, int iCount);
    void AddPotentialSurname(const TFullFIO& FIO, const TFIOOccurenceInText& group);
    void AddFIO(const TNameCluster& cluster );
    void Reset();
    bool Run(bool doMerge = true);
    const TVector<TNameCluster>& GetFioCluster(EFIOType type) const { return FIOClusterVectors[type];};
    void ClearCluster(EFIOType type) { FIOClusterVectors[type].clear(); }

protected:
    void MergeEqual(TVector<TNameCluster>& fio_vector);
    void MergeWithInitials(EFIOType type);
    //void ChangeFirstName(EFIOType type);
    void MergeFIOandF();
    void MergeIOandI();
    void MergeFIandF();
    void MergeFIOandFI();
    void MergeFIOandI();
    void MergeFIOandIO();
    void MergeFIandI();
    void MergeFIandIO();
    void MergeFIandFIin();
    void MergeFIOandFIinOin();
    void MergeFIOandPotentialSurnames();
    void MergeFIandPotentialSurnames();

    static bool Merge(TVector<TNameCluster>& to_vector,
                      TVector<TNameCluster>& from_vector,
                      FNameClusterOrder order, bool bDelUsedFios = false);
    static bool Merge(TVector<TNameCluster>& vector1,
                      TVector<TNameCluster>& vector2,
                      TVector<TNameCluster>& res_vector,
                      FNameClusterOrder order, bool preserveType, bool bDelUsedFios);

    static bool MergeWithCluster(TVector<TNameCluster>& to_vector,
                                 TVector<TNameCluster>& clustersToCompare,
                                 const TNameCluster& cluster,
                                 FNameClusterOrder order,
                                 bool preserveType);

    void Relocate(EFIOType index);
    static void ChangePatronymicToSurname(TNameCluster& cluster);
    static void TryToMerge(TVector<TNameCluster>& to_vector,
                           TVector<TNameCluster>& from_vector,
                           FNameClusterOrder order, TSet<size_t>& to_add,
                           bool bCheckFoundPatronymic = false);
    void ResolvePatronymicInFIO();
    void ResolvePatronymicInIO();

    static bool EnlargeIObyF(const TFullFIO& FIO, const TFIOOccurenceInText& group);
    void DeleteUsedFIOS();

    void ResolveFirstNameAndPatronymicContradictions();
    void ResolveFirstNameAndPatronymicContradictions(
            const TSetOfFioOccurnces& SetOfFioOccurnces);

    void ResolveSurnameAndFemMuscContradictions();
    void ResolveSurnameAndFemMuscContradictions(
            const TSetOfFioOccurnces& SetOfFioOccurnces);
    void FindClusyersForTheseOccurences(
            const TSetOfFioOccurnces& SetOfFioOccurnces,
            TVector<TVector<SCluster> >& setOfFioOccurncesNameVariants);

    void RestoreOriginalFio(TVector<SCluster>& nameVariants,
                            const TFIOOccurenceInText& fioEntry);

    void AddOccurenceToAllFios(const TFIOOccurenceInText& group);
    void AddSinglePredictedSurnamesToAllFios(const TVector<TNameCluster>& fioClusters);

    void AssignLocalSurnamePardigmID();
    bool SimilarSurnames(const TNameCluster& cl1, const TNameCluster& cl2 );

protected:
    TVector<TNameCluster>    FIOClusterVectors[FIOTypeCount];
    TVector<TNameCluster>    PotentialSurnames;
    TVector<TNameCluster>    FIOwithSuspiciousPatronymic;;
    TVector<TNameCluster>    IOwithSuspiciousPatronymic;;

    bool m_bResolveContradictions;
    const IFullTextForFioExtractor* FullText;
    TMap<TWordsPairInText, TSetOfFioOccurnces> AllFios;
};

