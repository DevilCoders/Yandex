#include "fioclusterbuilder.h"
#include <library/cpp/charset/wide.h>

using namespace std;

/*-----------------class TNameCluster-----------------*/

TNameCluster::TNameCluster()
{
    ToDel = false;
    SurnameIsLemmaCount = 0;
}

TNameCluster::TNameCluster(const TNameCluster& NameCluster) : FullName(NameCluster.FullName)
{
    NameVariants = NameCluster.NameVariants;
    ToDel = NameCluster.ToDel;
    SurnameIsLemmaCount = NameCluster.SurnameIsLemmaCount;
}

TNameCluster::~TNameCluster()
{

}

bool TNameCluster::defaul_order(const TNameCluster& cluster2) const
{
    return FullName.default_order(cluster2.FullName);
}

bool TNameCluster::order_by_f(const TNameCluster& cluster2) const
{
    return FullName.order_by_f(cluster2.FullName);
}

bool TNameCluster::order_by_f_pointer(const TNameCluster* cluster2) const
{
    return FullName.order_by_f(cluster2->FullName);
}


bool TNameCluster::order_by_i(const TNameCluster& cluster2) const
{
    return FullName.order_by_i(cluster2.FullName);
}

bool TNameCluster::order_by_o(const TNameCluster& cluster2) const
{
    return FullName.order_by_o(cluster2.FullName);
}

bool TNameCluster::order_by_fiIn(const TNameCluster& cluster2) const
{
    return FullName.order_by_fiIn(cluster2.FullName);
}

bool TNameCluster::order_by_fio(const TNameCluster& cluster2) const
{
    return FullName.order_by_fio(cluster2.FullName);
}

bool TNameCluster::order_by_io(const TNameCluster& cluster2) const
{
    return FullName.order_by_io(cluster2.FullName);
}

bool TNameCluster::order_by_fi(const TNameCluster& cluster2) const
{
    return FullName.order_by_fi(cluster2.FullName);
}

bool TNameCluster::Gleiche(const TNameCluster& cluster) const
{
    return FullName.Gleiche(cluster.FullName);
}

bool TNameCluster::order_by_fiInoIn(const TNameCluster& cluster2) const
{
    return FullName.order_by_fiInoIn(cluster2.FullName);
}

bool TNameCluster::order_by_fioIn(const TNameCluster& cluster2) const
{
    return FullName.order_by_fioIn(cluster2.FullName);
}


TString TNameCluster::ToString() const
{
    return "";
    //CString strRes;
    //strRes += "\t{\n";
    //if( !FullName.Surname.empty() )
    //{
    //    strRes += "\t\t";
    //    strRes += FullName.Surname;
    //    strRes += "\n";
    //}
    //if( !FullName.Name.empty() )
    //{
    //    strRes += "\t\t";
    //    strRes += FullName.Name;
    //    strRes += "\n";
    //}
    //if( !FullName.Patronymic.empty() )
    //{
    //    strRes += "\t\t";
    //    strRes += FullName.Patronymic;
    //    strRes += "\n";
    //}

    //strRes += "\t\t";
    //set<TFIOOccurenceInText>::const_iterator it = NameVariants.begin();
    //for( ; it != NameVariants.end() ; it++ )
    //{
    //    const TFIOOccurenceInText& fioWP = *it;
    //    CString strAddress;
    //    strAddress.Format("%d[%d, %d]",fioWP.GetSentNum(),fioWP.FirstWord(), fioWP.LastWord());
    //    strRes += strAddress;
    //    strRes += " ";
    //
    //}
    //strRes += "\n\t}";
    //return strRes;
}

void TNameCluster::Merge(const TNameCluster& cluster, bool bUniteCoordinate /*=true*/)
{
    // Note that the this->GetType() can be changed here
    // and the following invariant is NOT hold:
    //    ∀ {a,b,c} ⊂ TNameCluster : a.GetType() == b.GetType() => a.Merge(c).GetType() == b.Merge(c).GetType()
    FullName.FoundSurname = ( cluster.FullName.FoundSurname || FullName.FoundSurname );

    if( (FullName.Genders.any()) &&
        (cluster.FullName.Genders.any()) )
    {
        FullName.Genders &= cluster.FullName.Genders;
    }
    else
    {
        FullName.Genders |= cluster.FullName.Genders;
    }

    if( FullName.InitialName )
    {
        if( !cluster.FullName.Name.empty() )
        {
            FullName.Name = cluster.FullName.Name;
            FullName.InitialName = cluster.FullName.InitialName;
        }
    }
    else
    {
        if( FullName.Name.empty() &&  !cluster.FullName.Name.empty() )
            FullName.Name = cluster.FullName.Name;
    }

    MergeSurname(cluster, false);

    if( FullName.InitialPatronymic )
    {
        if( !cluster.FullName.Patronymic.empty() && !cluster.FullName.InitialPatronymic )
        {
            FullName.Patronymic = cluster.FullName.Patronymic;
            FullName.InitialPatronymic = cluster.FullName.InitialPatronymic;
        }
    }
    else
    {
        if( FullName.Patronymic.empty() && !cluster.FullName.Patronymic.empty() )
            FullName.Patronymic = cluster.FullName.Patronymic;
    }

    SurnameIsLemmaCount += cluster.SurnameIsLemmaCount;

    if( bUniteCoordinate )
        UniteCoordinate(cluster);
}

void TNameCluster::UniteCoordinate(const TNameCluster& cluster) {
    NameVariants.insert(cluster.NameVariants.begin(), cluster.NameVariants.end());
//    std::set_union(NameVariants.begin(), NameVariants.end(),
//                   cluster.NameVariants.begin(), cluster.NameVariants.end(),
//                   std::inserter(NameVariants, NameVariants.begin()));
}

void TNameCluster::MergePreservingType(const TNameCluster& cluster, bool bUniteCoordinate /*=true*/) {
    MergeSurname(cluster, true);
    if (bUniteCoordinate)
        UniteCoordinate(cluster);
}

void TNameCluster::MergeSurname(const TNameCluster& cluster, bool preservingType) {
    if( FullName.Surname.length() == 0 )
    {
        if( cluster.FullName.Surname.length() > 0 && !preservingType )
            FullName.Surname = cluster.FullName.Surname;
    }
    else
    {
        if( cluster.FullName.Surname.length() > 0 )
        {
            if( !FullName.SurnameLemma && cluster.FullName.SurnameLemma )
            {
                FullName.Surname = cluster.FullName.Surname;
                FullName.SurnameLemma = true;
            }
            else
            {
                if( !FullName.SurnameLemma ||
                    (FullName.SurnameLemma && cluster.FullName.SurnameLemma) )
                {
                    if( (FullName.SurnameLemma && cluster.FullName.SurnameLemma) )
                    {
                        if( SurnameIsLemmaCount < cluster.SurnameIsLemmaCount )
                            FullName.Surname = cluster.FullName.Surname;
                    }
                    if( !FullName.SurnameLemma ||
                        (SurnameIsLemmaCount == cluster.SurnameIsLemmaCount) )
                    {
                        if( cluster.FullName.Surname.length() < FullName.Surname.length() )
                            FullName.Surname = cluster.FullName.Surname;
                        else
                            if( ( cluster.FullName.Surname.length() == FullName.Surname.length())  &&
                                ( cluster.FullName.Surname < FullName.Surname ) )
                                FullName.Surname = cluster.FullName.Surname;
                    }
                }

            }
        }
    }
}

bool TNameCluster::operator<(const TNameCluster& Cluster) const
{
    return FullName < Cluster.FullName;
}

bool TNameCluster::operator==(const TNameCluster& Cluster) const
{
    return FullName == Cluster.FullName;
}

/*-----------------class TFioClusterBuilder-----------------*/

TFioClusterBuilder::TFioClusterBuilder(const IFullTextForFioExtractor* fullText)
    : FullText(fullText)
{
    m_bResolveContradictions = false;
}

TFioClusterBuilder::~TFioClusterBuilder()
{

}

bool TFioClusterBuilder::SimilarSurnames(const TNameCluster& cl1, const TNameCluster& cl2 )
{
//    TSet<TFIOOccurenceInText>::const_iterator it1 = cl1.NameVariants.begin();
//    TSet<TFIOOccurenceInText>::const_iterator it2 = cl2.NameVariants.begin();
//    if (FullText) {
//        for( ; it1 != cl1.NameVariants.end() ; it1++ ) {
//            for( ; it2 != cl2.NameVariants.end() ; it2++ ) {
//                const TFIOOccurenceInText& oc1 = *it1;
//                const TFIOOccurenceInText& oc2 = *it2;
//                if( oc1.NameMembers[Surname].IsValid() &&  oc2.NameMembers[Surname].IsValid() ) {
//                    if( FullText->HasCommonLemmas(oc1.GetSentNum(), oc1.NameMembers[Surname].WordNum, oc2.GetSentNum(), oc2.NameMembers[Surname].WordNum) )
//                        return true;
//                }
//            }
//        }
//    }
    return cl1.GetFullName().Surname == cl2.GetFullName().Surname;
}

void TFioClusterBuilder::AddPotentialSurname(const TFullFIO& FIO, const TFIOOccurenceInText& group)
{
    if( FIO.IsEmpty() )
        return;

    TNameCluster  NameCluster;
    NameCluster.FullName = FIO;
    //NameCluster.FullName.MakeLower();
    NameCluster.NameVariants.insert(group);
    PotentialSurnames.push_back(NameCluster);
}

void TFioClusterBuilder::AddFIO(const TFullFIO& FIO, const TFIOOccurenceInText& group, bool bSuspiciousPatronymic, bool bSurnameLemma, int iSurnameIsLemmaCount)
{
    if( FIO.IsEmpty() )
        return;

    TNameCluster  NameCluster;
    NameCluster.FullName = FIO;
    NameCluster.FullName.Genders = group.GetGrammems() & NSpike::AllGenders;

    NameCluster.FullName.SurnameLemma = bSurnameLemma;

    if(        !FIO.FoundSurname &&
            !NameCluster.FullName.Surname.empty() &&
            !NameCluster.FullName.InitialName &&
            !NameCluster.FullName.SurnameLemma)
    {
        TGramBitSet grammems = TGramBitSet(gNominative, gSingular);
        NameCluster.FullName.SurnameLemma = (( group.GetGrammems() &  grammems) == grammems);

    }


    if( bSurnameLemma )
        NameCluster.SurnameIsLemmaCount = iSurnameIsLemmaCount;

    NameCluster.NameVariants.insert(group);
    if( !bSuspiciousPatronymic )
        FIOClusterVectors[NameCluster.FullName.GetType()].push_back(NameCluster);
    else
    {
        if( FIO.GetType() == FIOname )
            FIOwithSuspiciousPatronymic.push_back(NameCluster);
        if( FIO.GetType() == IOname )
            IOwithSuspiciousPatronymic.push_back(NameCluster);
    }
    AddOccurenceToAllFios(group);
}

void TFioClusterBuilder::AddOccurenceToAllFios(const TFIOOccurenceInText& group)
{
    TWordsPairInText addr(group.GetSentNum(), group); //TSetOfFioOccurnces setOfFioOccurnces(group.GetSentNum(), group);
    TMap<TWordsPairInText, TSetOfFioOccurnces>::iterator it = AllFios.find(addr);
    if( it == AllFios.end() )
    {
        TSetOfFioOccurnces setOfFioOccurnces;
        setOfFioOccurnces.Occurences.push_back(group);
        AllFios[addr] = setOfFioOccurnces;
    }
    else
    {

        it->second.Occurences.push_back(group);
    }

}

//void TFioClusterBuilder::ChangeFirstName(EFIOType type)
//{
    //for(int i = 0 ; i < FIOClusterVectors[type].size()  ; i++ )
    //{
    //    if( FIOClusterVectors[type][i].FullName.Name.empty() )
    //        return;
    //    CString strName = m_pMorph->m_pGramInfo->GetFullFirstName(FIOClusterVectors[type][i].FullName.Name);
    //    if( strName != FIOClusterVectors[type][i].FullName.Name )
    //    {
    //        if( m_pMorph->GetMorph(Russian)->GetMorphVersion() >= DialingMorph )
    //        {
    //            vector<CMorphRes> out;
    //            const CAfMorphRusDialing* pMorph = m_pMorph->GetMorph(Russian)->AsAfMorphRusDialing();
    //            if( pMorph->m_pMorph->Find(strName, out) )
    //            {
    //                int j = 0 ;
    //                for(; j < out.size() ; j++ )
    //                    if( strName == out[j].lemm )
    //                        break;
    //                if( j >= out.size() )
    //                    strName.Empty();
    //            }
    //            else
    //                strName.Empty();
    //        }
    //    }
    //    if( !strName.empty() )
    //        FIOClusterVectors[type][i].FullName.Name = strName;
    //}
//}

void TFioClusterBuilder::MergeEqual(TVector<TNameCluster>& fio_vector)
{

    TVector<TNameCluster> fio_vector_save;
    fio_vector_save.reserve(fio_vector.size());
    fio_vector_save = fio_vector;
    fio_vector.clear();
    bool bFirst = true;
    for(size_t i = 0 ; i < fio_vector_save.size() ; i++ )
    {
        if( bFirst )
        {
            fio_vector.push_back(fio_vector_save[i]);
            bFirst = false;
            continue;
        }

        int k = (int)fio_vector.size() -1;
        bool bMerged = false;
        while( (k >= 0) && (fio_vector[k].FullName == fio_vector_save[i].FullName) )
        {
            if( fio_vector[k].FullName.Gleiche(fio_vector_save[i].FullName) )
            {
                fio_vector[k].Merge(fio_vector_save[i]);
                bMerged = true;
            }
            k--;
        }
        if( !bMerged )
            fio_vector.push_back(fio_vector_save[i]);
    }
}

void TFioClusterBuilder::MergeIOandI()
{
    Merge(FIOClusterVectors[IOname], FIOClusterVectors[Iname], &TNameCluster::order_by_i);
    sort(FIOClusterVectors[IOname].begin(), FIOClusterVectors[IOname].end());
}

void TFioClusterBuilder::MergeFIOandF()
{
    if( Merge(FIOClusterVectors[FIOname], FIOClusterVectors[Fname], &TNameCluster::order_by_f) )
        sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());

    if( Merge(FIOClusterVectors[FIinOinName], FIOClusterVectors[Fname], &TNameCluster::order_by_f) )
        sort(FIOClusterVectors[FIinOinName].begin(), FIOClusterVectors[FIinOinName].end());

    if( Merge(FIOClusterVectors[FIOinName], FIOClusterVectors[Fname], &TNameCluster::order_by_f) )
        sort(FIOClusterVectors[FIOinName].begin(), FIOClusterVectors[FIOinName].end());
}


void TFioClusterBuilder::AddSinglePredictedSurnamesToAllFios(const TVector<TNameCluster>& fioClusters)
{
    for( size_t i = 0 ; i < fioClusters.size() ; i++ )
    {
        const TNameCluster& cluster = fioClusters[i];
        if( cluster.GetFullName().FoundSurname )
            continue;
        TSet<TFIOOccurenceInText>::const_iterator it = cluster.NameVariants.begin();
        for(; it != cluster.NameVariants.end() ; it++)
        {
            const TFIOOccurenceInText& group = *it;
            if( group.NameMembers[Surname].IsValid() &&
                !group.NameMembers[FirstName].IsValid() &&
                !group.NameMembers[InitialName].IsValid() )
            {
                AddOccurenceToAllFios(group);
            }
        }

    }
}

void TFioClusterBuilder::MergeFIandPotentialSurnames()
{
    sort(PotentialSurnames.begin(), PotentialSurnames.end());
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());
    if( Merge(FIOClusterVectors[FIname], PotentialSurnames, &TNameCluster::order_by_f) )
        //добавим к AllFios - раньше не добавили, так как PotentialSurnames может быть слишком много
        //зачем лишнее добавлять
        AddSinglePredictedSurnamesToAllFios(FIOClusterVectors[FIname]);
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());

    sort(FIOClusterVectors[FIinName].begin(), FIOClusterVectors[FIinName].end());
    if( Merge(FIOClusterVectors[FIinName], PotentialSurnames, &TNameCluster::order_by_f) )
        AddSinglePredictedSurnamesToAllFios(FIOClusterVectors[FIinName]);
    sort(FIOClusterVectors[FIinName].begin(), FIOClusterVectors[FIinName].end());

}

void TFioClusterBuilder::MergeFIOandPotentialSurnames()
{
    sort(PotentialSurnames.begin(), PotentialSurnames.end());
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
    if( Merge(FIOClusterVectors[FIOname], PotentialSurnames, &TNameCluster::order_by_f) )
        AddSinglePredictedSurnamesToAllFios(FIOClusterVectors[FIOname]);
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());

    sort(FIOClusterVectors[FIOinName].begin(), FIOClusterVectors[FIOinName].end());
    if( Merge(FIOClusterVectors[FIOinName], PotentialSurnames, &TNameCluster::order_by_f) )
        AddSinglePredictedSurnamesToAllFios(FIOClusterVectors[FIOinName]);
    sort(FIOClusterVectors[FIOinName].begin(), FIOClusterVectors[FIOinName].end());

    sort(FIOClusterVectors[FIinOinName].begin(), FIOClusterVectors[FIinOinName].end());
    if( Merge(FIOClusterVectors[FIinOinName], PotentialSurnames, &TNameCluster::order_by_f) )
        AddSinglePredictedSurnamesToAllFios(FIOClusterVectors[FIinOinName]);
    sort(FIOClusterVectors[FIinOinName].begin(), FIOClusterVectors[FIinOinName].end());

}


void TFioClusterBuilder::MergeFIandF()
{
    if( Merge(FIOClusterVectors[FIname], FIOClusterVectors[Fname], &TNameCluster::order_by_f) )
        sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());

    if( Merge(FIOClusterVectors[FIinName], FIOClusterVectors[Fname], &TNameCluster::order_by_f) )
        sort(FIOClusterVectors[FIinName].begin(), FIOClusterVectors[FIinName].end());
}

void TFioClusterBuilder::MergeFIOandFI()
{
    if( Merge(FIOClusterVectors[FIOname], FIOClusterVectors[FIname], &TNameCluster::order_by_fi) )
        sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());

    //if( Merge(FIOClusterVectors[FIOname], FIOClusterVectors[FIinName], &TNameCluster::order_by_fi) )
    //    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());

    if( Merge(FIOClusterVectors[FIinOinName], FIOClusterVectors[FIinName], &TNameCluster::order_by_fiIn) )
        sort(FIOClusterVectors[FIinOinName].begin(), FIOClusterVectors[FIinOinName].end());

    if( Merge(FIOClusterVectors[FIinOinName], FIOClusterVectors[FIname], FIOClusterVectors[FIOinName], &TNameCluster::order_by_fiIn, false, false) ) {
        Relocate(FIOinName);
        for (int i = 0; i < FIOTypeCount; i++)
           sort(FIOClusterVectors[i].begin(), FIOClusterVectors[i].end());
    }

    if( Merge(FIOClusterVectors[FIOinName], FIOClusterVectors[FIinName], &TNameCluster::order_by_fiIn) )
        sort(FIOClusterVectors[FIOinName].begin(), FIOClusterVectors[FIOinName].end());

    if( Merge(FIOClusterVectors[FIOinName], FIOClusterVectors[FIname], &TNameCluster::order_by_fi) )
        sort(FIOClusterVectors[FIOinName].begin(), FIOClusterVectors[FIOinName].end());
}

void TFioClusterBuilder::MergeFIandFIin()
{
    if( Merge(FIOClusterVectors[FIname], FIOClusterVectors[FIinName], &TNameCluster::order_by_fiIn,false) )
        sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());
    if( Merge(FIOClusterVectors[FIOname], FIOClusterVectors[FIinName], &TNameCluster::order_by_fiIn, true) )
        sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
}

void TFioClusterBuilder::MergeFIOandFIinOin()
{

    if( Merge(FIOClusterVectors[FIOinName], FIOClusterVectors[FIinOinName], &TNameCluster::order_by_fiInoIn, false) )
        sort(FIOClusterVectors[FIOinName].begin(), FIOClusterVectors[FIOinName].end());


    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end(), std::mem_fn(&TNameCluster::order_by_fiInoIn));
    if( Merge(FIOClusterVectors[FIOname], FIOClusterVectors[FIinOinName], &TNameCluster::order_by_fiInoIn, true) )
        sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());

    Merge(FIOClusterVectors[FIOname], FIOClusterVectors[FIOinName], &TNameCluster::order_by_fio, true);

    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
}



void TFioClusterBuilder::MergeFIandI()
{
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end(), std::mem_fn(&TNameCluster::order_by_i));
    Merge(FIOClusterVectors[FIname], FIOClusterVectors[Iname], &TNameCluster::order_by_i);
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());
}

void TFioClusterBuilder::MergeFIOandI()
{
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end(), std::mem_fn(&TNameCluster::order_by_i));
    Merge(FIOClusterVectors[FIOname], FIOClusterVectors[Iname], &TNameCluster::order_by_i);
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
}

void TFioClusterBuilder::MergeFIOandIO()
{
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end(), std::mem_fn(&TNameCluster::order_by_io));
    Merge(FIOClusterVectors[FIOname], FIOClusterVectors[IOname], &TNameCluster::order_by_io);
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
}

void TFioClusterBuilder::MergeFIandIO()
{
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end(), std::mem_fn(&TNameCluster::order_by_i));

    //Merge(FIname, IOname, &TNameCluster::order_by_i);
    //так как при слиянии FIname IOname мы получаем FIOname, то нужно из вектора с
    //FIname удалить те, которые стали FIOname и перепмсать их в вектор с FIOname
    TVector<TNameCluster>::iterator res_it;

    TVector<TNameCluster>::iterator fio_it = FIOClusterVectors[FIname].begin();

    TSet<size_t> FItoDel;

    for(int i = 0 ; i < (int)FIOClusterVectors[IOname].size() ; i++ )
    {
        TNameCluster& cluster = FIOClusterVectors[IOname][i];


        bool bDel = false;

        fio_it = FIOClusterVectors[FIname].begin();

        res_it = lower_bound(fio_it, FIOClusterVectors[FIname].end(), cluster, std::mem_fn(&TNameCluster::order_by_i));

        if( (res_it == FIOClusterVectors[FIname].end()) )//|| (res_it->FullName.Surname != cluster.FullName.Surname) )
            continue;
        fio_it = res_it;

        TVector<TNameCluster> newClusters;
        TVector<size_t> delFI;
        while( (fio_it != FIOClusterVectors[FIname].end()) && ( !cluster.order_by_i(*fio_it)  && !(*fio_it).order_by_i(cluster)) )
        {
            if( !cluster.Gleiche(*fio_it) )
            {
                fio_it++;
                continue;
            }
            TNameCluster new_cluster(*fio_it);
            new_cluster.Merge(cluster, false);

            if( new_cluster.FullName.GetType() == FIOname )
            {
                //может возникнуть ситуация, что то FIOname, которое мы получаем, уже было в тексте, мы уже к
                //нему подливали и FIname и IOname, но так как мы их не удаляем после сливания, то может
                //возникнуть такая ситуевина (например,  Александр Иванович, Александр Иванов, Александр Иванович Иванов)
                if( find(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end(), new_cluster ) == FIOClusterVectors[FIOname].end() )
                {
                    newClusters.push_back(new_cluster);
                }
            }

            bDel = true;
            delFI.push_back(fio_it - FIOClusterVectors[FIname].begin());
            fio_it++;
        }

        //если несколько вариантов (Сергей Петров, СЕРГЕЙ ВЛАДИЛЕНОВИЧ, Сергей Иванов),
        //то не сливаем
        if( newClusters.size() > 1 )
        {
            delFI.clear();
            bDel = false;
        }
        else
        {
            FIOClusterVectors[FIOname].insert(FIOClusterVectors[FIOname].end(), newClusters.begin(), newClusters.end());
        }


        if( bDel )
        {
            for( size_t k =0 ; k < delFI.size() ; k++ )
                FItoDel.insert(delFI[k]);
            FIOClusterVectors[IOname].erase(FIOClusterVectors[IOname].begin() + i);
            i--;
        }

        if( res_it == FIOClusterVectors[FIname].end() )
            break;
    }

    TSet<size_t>::iterator set_it = FItoDel.end();
    if( FItoDel.size() )
    {
        do
        {
            FIOClusterVectors[FIname].erase(FIOClusterVectors[FIname].begin() + *(--set_it));
        }while(set_it != FItoDel.begin());
    }

    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
}

void TFioClusterBuilder::Relocate(EFIOType index) {
    for (size_t i = 0; i < FIOClusterVectors[index].size(); ) {
        EFIOType actualType = FIOClusterVectors[index][i].GetFullName().GetType();
        if (actualType == index) {
            ++i;
        } else {
            FIOClusterVectors[actualType].push_back(FIOClusterVectors[index][i]);
            if (i + 1 != FIOClusterVectors[index].size())
                FIOClusterVectors[index][i] = FIOClusterVectors[index].back();
            FIOClusterVectors[index].pop_back();
        }
    }
}

bool TFioClusterBuilder::Merge(TVector<TNameCluster>& clustersToCompare, TVector<TNameCluster>& from_vector, TVector<TNameCluster>& to_vector, FNameClusterOrder order, bool bDelUsedFios, bool preserveType)
{
    bool bMergedSomething = false;
    for(int i = 0 ; i < (int)from_vector.size() ; i++ )
    {
        TNameCluster& cluster = from_vector[i];
        if (MergeWithCluster(to_vector, clustersToCompare, cluster, order, preserveType)) {
            cluster.ToDel = true;
            bMergedSomething = true;
        }
    }

    if( bDelUsedFios  )
    {
        for(int j = (int)from_vector.size() - 1 ; j >= 0 ; j-- )
            if( from_vector[j].ToDel )
                from_vector.erase(from_vector.begin() + j);
    }

    return bMergedSomething ;
}

bool TFioClusterBuilder::Merge(TVector<TNameCluster>& to_vector, TVector<TNameCluster>& from_vector, FNameClusterOrder order, bool bDelUsedFios)
{
    TVector<TNameCluster> clustersToCompare;
    to_vector.swap(clustersToCompare);

    bool bRes = Merge(clustersToCompare, from_vector, to_vector, order, bDelUsedFios, true);

    for(size_t i = 0 ; i < clustersToCompare.size() ; i++ )
    {
        if( !clustersToCompare[i].ToDel ) {
            to_vector.push_back(clustersToCompare[i]);
            Y_ASSERT(to_vector[0].FullName.GetType() == to_vector.back().FullName.GetType());
        }
    }
    return bRes;
}

template<class T, class Order>
static bool EqualByOrder(const T& lhs, const T& rhs, Order order) {
    return order(lhs, rhs) == order(rhs, lhs);
}

bool TFioClusterBuilder::MergeWithCluster(TVector<TNameCluster>& to_vector, TVector<TNameCluster>& clustersToCompare, const TNameCluster& cluster, FNameClusterOrder order, bool preserveType)
{
    TVector<TNameCluster>::iterator fio_it = clustersToCompare.begin();
    TVector<TNameCluster>::iterator fio_it_end = clustersToCompare.end();

    bool bMergedSomething = false;

    fio_it = lower_bound(fio_it, fio_it_end, cluster, std::mem_fn(order));

    if( (fio_it == fio_it_end) )//|| (res_it->FullName.Surname != cluster.FullName.Surname) )
        return false;


    while(    (fio_it != fio_it_end) &&
            EqualByOrder(cluster, *fio_it, std::mem_fn(order)) )
    {
        if( !cluster.Gleiche(*fio_it) )
        {
            fio_it++;
            continue;
        }

        to_vector.push_back(*fio_it);
        if (preserveType) {
            to_vector.back().MergePreservingType(cluster);
            Y_ASSERT(fio_it->FullName.GetType() == to_vector.back().FullName.GetType());
        } else {
            to_vector.back().Merge(cluster);
        }
        if (0) {
            Cout << CharToWide(fio_it->FullName.ToString(), csYandex) << " merged into " << CharToWide(cluster.GetFullName().ToString(), csYandex) << Endl;
        }
        to_vector.back().ToDel = false;
        fio_it->ToDel = true;
        fio_it++;
        bMergedSomething = true;
    }

    return bMergedSomething;
}


void TFioClusterBuilder::Reset()
{
        for(int i = 0 ; i < FIOTypeCount ; i++ )
            FIOClusterVectors[(EFIOType)i].clear();

}

void TFioClusterBuilder::DeleteUsedFIOS()
{
    for(int i = 0 ; i < FIOTypeCount ; i++ )
    {
        for(int j = (int)FIOClusterVectors[(EFIOType)i].size() - 1 ; j >= 0 ; j-- )
            if( FIOClusterVectors[(EFIOType)i][j].ToDel )
                FIOClusterVectors[(EFIOType)i].erase(FIOClusterVectors[(EFIOType)i].begin() + j);
    }
}


void TFioClusterBuilder::TryToMerge(TVector<TNameCluster>& to_vector, TVector<TNameCluster>& from_vector, FNameClusterOrder order, TSet<size_t>& to_add, bool bCheckFoundPatronymic)
{
    TVector<TNameCluster>::iterator fio_it = to_vector.begin();
    TVector<TNameCluster>::iterator fio_it_end = to_vector.end();
    TVector<TNameCluster>::iterator res_it;
    TVector<TNameCluster> copies_with_initials;

    for(int i = 0 ; i < (int)from_vector.size() ; i++ )
    {
        TNameCluster& cluster = from_vector[i];
        if( !cluster.FullName.FoundPatronymic && bCheckFoundPatronymic )
            continue;

        fio_it = to_vector.begin();

        res_it = lower_bound(fio_it, fio_it_end, cluster, std::mem_fn(order));

        if( (res_it == fio_it_end) )
            continue;

        fio_it = res_it;

        if(  !(cluster.*order)(*fio_it)  && !((*fio_it).*order)(cluster) &&
            cluster.FullName.Patronymic != fio_it->FullName.Surname )
            to_add.insert(i);
    }
}

void TFioClusterBuilder::ChangePatronymicToSurname(TNameCluster& cluster)
{
    cluster.FullName.Surname = cluster.FullName.Patronymic;
    cluster.FullName.Patronymic.clear();

    if( cluster.FullName.FoundPatronymic )
        cluster.FullName.FoundSurname = true;
    else
        cluster.FullName.FoundSurname = false;

    cluster.FullName.FoundPatronymic = true;//восстановим значение по умолчанию

    TSet<TFIOOccurenceInText>::iterator it = cluster.NameVariants.begin();
    for( ; it != cluster.NameVariants.end() ; it++ )
    {
        TFIOOccurenceInText& address = (TFIOOccurenceInText&)(*it);
        if( !address.NameMembers[MiddleName].IsValid() )
            continue;

        if( address.NameMembers[Surname].WordNum == address.FirstWord() )
            address.ChangeFirstWord(address.FirstWord()+1);
        if( address.NameMembers[Surname].WordNum == address.LastWord() )
            address.ChangeLastWord(address.LastWord()-1);

        address.NameMembers[Surname] = address.NameMembers[MiddleName];
    }
}

void TFioClusterBuilder::ResolvePatronymicInFIO()
{
    if( FIOwithSuspiciousPatronymic.size() == 0 )
        return;
    TSet<size_t> added;

    sort(FIOwithSuspiciousPatronymic.begin(), FIOwithSuspiciousPatronymic.end());

    TVector<TNameCluster> fioWithPatronymicAsSurname = FIOwithSuspiciousPatronymic;
    for( int i = 0 ; i < fioWithPatronymicAsSurname.ysize() ; i++ )
        ChangePatronymicToSurname(fioWithPatronymicAsSurname[i]);
    sort(fioWithPatronymicAsSurname.begin(), fioWithPatronymicAsSurname.end());

    TryToMerge(FIOClusterVectors[FIname], fioWithPatronymicAsSurname, &TNameCluster::order_by_fi, added);
    TryToMerge(FIOClusterVectors[FIOname], fioWithPatronymicAsSurname, &TNameCluster::order_by_fi, added);

    for(int i = 0 ; i < (int)fioWithPatronymicAsSurname.size() ; i++ )
    {
        TNameCluster& cluster = FIOwithSuspiciousPatronymic[i];
        if( added.find(i) == added.end() )
        {
            for( int j = 0 ; j < FIOwithSuspiciousPatronymic.ysize(); j++ )
            {
                if( FIOwithSuspiciousPatronymic[j].NameVariants == cluster.NameVariants )
                {
                    FIOClusterVectors[FIOname].push_back(cluster);
                    break;
                }
            }
        }
        else
            FIOClusterVectors[FIname].push_back(cluster);

    }
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
}

void TFioClusterBuilder::ResolvePatronymicInIO()
{
    if( IOwithSuspiciousPatronymic.size() == 0 )
        return;

    TSet<size_t> added;

    sort(IOwithSuspiciousPatronymic.begin(), IOwithSuspiciousPatronymic.end());

    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end(), std::mem_fn(&TNameCluster::order_by_io));
    TryToMerge( FIOClusterVectors[FIOname], IOwithSuspiciousPatronymic, &TNameCluster::order_by_io, added);

    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end(), std::mem_fn(&TNameCluster::order_by_i));
    TryToMerge( FIOClusterVectors[FIname], IOwithSuspiciousPatronymic, &TNameCluster::order_by_i, added, true);

    for( size_t i = 0 ; i < IOwithSuspiciousPatronymic.size() ; i++ )
    {
        TNameCluster& cluster = IOwithSuspiciousPatronymic[i];
        if( added.find(i) == added.end() )
        {
            ChangePatronymicToSurname(cluster);
            FIOClusterVectors[FIname].push_back(cluster);
        }
        else
        {
            //мы его приписали в ф-ции AssignLocalSurnamePardigmID, в
            //надежде на то, что отчество может стать фамилией
            //не стало...
            cluster.FullName.LocalSuranamePardigmID = -1;
            FIOClusterVectors[IOname].push_back(cluster);
        }
    }
    sort(FIOClusterVectors[FIname].begin(), FIOClusterVectors[FIname].end());
    sort(FIOClusterVectors[FIOname].begin(), FIOClusterVectors[FIOname].end());
    sort(FIOClusterVectors[IOname].begin(), FIOClusterVectors[IOname].end());

}


void TFioClusterBuilder::RestoreOriginalFio(TVector<SCluster>& nameVariants, const TFIOOccurenceInText& fioEntry)
{
    TNameCluster newCluster = FIOClusterVectors[nameVariants[0].m_FioTYPE][nameVariants[0].m_iCluster];
    newCluster.NameVariants.clear();
    newCluster.NameVariants.insert(fioEntry);

    if( fioEntry.NameMembers[InitialPatronymic].IsValid() )
        newCluster.FullName.Patronymic = TString(newCluster.FullName.Patronymic[0]);
    else if( !fioEntry.NameMembers[MiddleName].IsValid() )
        newCluster.FullName.Patronymic.clear();

    if( fioEntry.NameMembers[InitialName].IsValid() )
        newCluster.FullName.Name= TString(newCluster.FullName.Name[0]);
    else if( !fioEntry.NameMembers[FirstName].IsValid() )
        newCluster.FullName.Name.clear();


    for(size_t j = 0 ; j < nameVariants.size() ; j++ )
    {
        TNameCluster& oldCluster = FIOClusterVectors[nameVariants[j].m_FioTYPE][nameVariants[j].m_iCluster];
        TSet<TFIOOccurenceInText>::iterator it = oldCluster.NameVariants.find(fioEntry);
        if( it != oldCluster.NameVariants.end() )
            oldCluster.NameVariants.erase(it);
    }

    FIOClusterVectors[newCluster.FullName.GetType()].push_back(newCluster);
}

void TFioClusterBuilder::ResolveFirstNameAndPatronymicContradictions()
{
    TMap<TWordsPairInText, TSetOfFioOccurnces>::const_iterator it = AllFios.begin();
    for( ; it != AllFios.end() ; it++ )
        //рассматриваем одну цепочку в предложении с, возможно, несколькими фио
        //например "Евгения Петрова" - один TSetOfFioOccurnces,
        //в котором TSetOfFioOccurnces.Occurences - 2 варианта
        ResolveFirstNameAndPatronymicContradictions(it->second);
}

void TFioClusterBuilder::ResolveSurnameAndFemMuscContradictions()
{
    TMap<TWordsPairInText, TSetOfFioOccurnces>::const_iterator it = AllFios.begin();
    for( ; it != AllFios.end() ; it++ )
        //рассматриваем одну цепочку в предложении с, возможно, несколькими фио
        //например "Евгения Петрова" - один TSetOfFioOccurnces,
        //в котором TSetOfFioOccurnces.Occurences - 2 варианта
        ResolveSurnameAndFemMuscContradictions(it->second);
}

void TFioClusterBuilder::FindClusyersForTheseOccurences(const TSetOfFioOccurnces& SetOfFioOccurnces, TVector<TVector<SCluster> >& setOfFioOccurncesNameVariants)
{
    setOfFioOccurncesNameVariants.resize(SetOfFioOccurnces.Occurences.size());

    for( size_t i = 0 ; i < SetOfFioOccurnces.Occurences.size() ; i++)
    {
        const TFIOOccurenceInText& fioEntry = SetOfFioOccurnces.Occurences[i];
        TVector<SCluster>& nameVariants = setOfFioOccurncesNameVariants[i];

        for( int j = 0 ;j < FIOTypeCount ; j++)
        {
            for( size_t k = 0 ; k < FIOClusterVectors[j].size() ; k++ )
            {
                TNameCluster& cluster = FIOClusterVectors[j][k];
                TSet<TFIOOccurenceInText>::iterator it = cluster.NameVariants.find(fioEntry);
                if( it != cluster.NameVariants.end() )
                {
                    SCluster clusterAddr;
                    clusterAddr.m_FioTYPE = (EFIOType)j;
                    clusterAddr.m_iCluster = (int)k;
                    nameVariants.push_back(clusterAddr);
                }
            }
        }
    }
}

void TFioClusterBuilder::ResolveSurnameAndFemMuscContradictions(const TSetOfFioOccurnces& SetOfFioOccurnces)
{
    TVector<TVector<SCluster> > setOfFioOccurncesNameVariants;

    //посмотрим в какие кластера вошли разные омонимы этого ФИО (SetOfFioOccurnces - это
    //множество различных омонимов ФИО из одного(!) вхождения)
    FindClusyersForTheseOccurences(SetOfFioOccurnces, setOfFioOccurncesNameVariants);

    int iMaxCount = -1;
    int iMinCount = -1;

    //посмотрим есть ли такие омонимы, которые ни с кем не слились, а также
    //такие, которые еще с кем-то слились
    for( size_t  i = 0 ; i < setOfFioOccurncesNameVariants.size() ; i++)
    {
        if( setOfFioOccurncesNameVariants[i].size() > 0 )
        {
            for( size_t j = 0 ; j < setOfFioOccurncesNameVariants[i].size() ; j++ )
            {
                SCluster cl = setOfFioOccurncesNameVariants[i][j];
                TNameCluster& nameCluster = FIOClusterVectors[cl.m_FioTYPE][cl.m_iCluster];
                if( iMaxCount == -1 || iMaxCount < (int)nameCluster.NameVariants.size() )
                    iMaxCount = nameCluster.NameVariants.size();
                if( iMinCount == -1 || iMinCount > (int)nameCluster.NameVariants.size() )
                    iMinCount = nameCluster.NameVariants.size();
            }
        }
    }

    //если как есть такие омонимы, кторые еще с кем-то слились, то удалим все те, которые ни с кем не слились
    if( (iMinCount == 1) && (iMaxCount > 1) )
    {
        for( size_t  i = 0 ; i < setOfFioOccurncesNameVariants.size() ; i++)
        {
            //if(setOfFioOccurncesNameVariants[i].size() == 1)
                for( size_t j = 0 ; j < setOfFioOccurncesNameVariants[i].size() ; j++ )
                {
                    SCluster cl = setOfFioOccurncesNameVariants[i][j];
                    TNameCluster& nameCluster = FIOClusterVectors[cl.m_FioTYPE][cl.m_iCluster];
                    if( nameCluster.NameVariants.size() == 1 )
                    {
                        nameCluster.NameVariants.clear();
                    }
                    else
                    {
                        TSet<TFIOOccurenceInText> variants = nameCluster.NameVariants;
                        nameCluster.NameVariants.clear();
                        TSet<TFIOOccurenceInText>::iterator it = variants.begin();
                        for(; it != variants.end() ; it++ )
                        {
                            TFIOOccurenceInText FIOOccurenceInText = *it;
                            FIOOccurenceInText.SetRealPair();
                            nameCluster.NameVariants.insert(FIOOccurenceInText);
                        }
                    }
                }
        }
    }
}

void TFioClusterBuilder::ResolveFirstNameAndPatronymicContradictions(const TSetOfFioOccurnces& SetOfFioOccurnces)
{
    TVector<TVector<SCluster> > setOfFioOccurncesNameVariants;
    bool bHasContradictions = false;
    setOfFioOccurncesNameVariants.resize(SetOfFioOccurnces.Occurences.size());
    for( size_t i = 0 ; i < SetOfFioOccurnces.Occurences.size() ; i++)
    {
        const TFIOOccurenceInText& fioEntry = SetOfFioOccurnces.Occurences[i];
        TVector<SCluster>& nameVariants = setOfFioOccurncesNameVariants[i];
        for( int j = 0 ;j < FIOTypeCount ; j++)
        {
            if( (j != FIOname) && (j != FIname) &&
                (j != FIinOinName) && (j != FIinName) && (j != FIOinName))
                continue;
            for( size_t k = 0 ; k < FIOClusterVectors[j].size() ; k++ )
            {
                TNameCluster& cluster = FIOClusterVectors[j][k];
                //if( cluster.NameVariants.size() > 1 )
                {
                    TSet<TFIOOccurenceInText>::iterator it = cluster.NameVariants.find(fioEntry);
                    if( it != cluster.NameVariants.end() )
                    {
                        SCluster clusterAddr;
                        clusterAddr.m_FioTYPE = (EFIOType)j;
                        clusterAddr.m_iCluster = (int)k;
                        nameVariants.push_back(clusterAddr);
                    }
                }
            }
        }
        if( nameVariants.size() >= 2 )
            bHasContradictions = true;

    }

    if( bHasContradictions )
        //если хотя бы один омоним фио противоречив, то
        //удаляем все полученные слияния
        //то есть, если, например, у фио "Евгения Петрова" м.р. совпал с двумя противоречивыми фио, а ж.р.
        //совпал с непротиворечивыми фио, то не считаем, что это повод удалить мужской омоним,
        //и отменям все расширения фио для обоих омонимов
        for( size_t  i = 0 ; i < setOfFioOccurncesNameVariants.size() ; i++)
        {
            if( setOfFioOccurncesNameVariants[i].size() > 0 )
                RestoreOriginalFio(setOfFioOccurncesNameVariants[i], SetOfFioOccurnces.Occurences[i]);
        }
}

void TFioClusterBuilder::AssignLocalSurnamePardigmID()
{
    TVector<TNameCluster*> allClusters;
    size_t i = 0;
    for( ; i < FIOTypeCount ; i++ )
        for( size_t j = 0 ; j < FIOClusterVectors[(EFIOType)i].size() ; j++ )
        {
            if( !FIOClusterVectors[(EFIOType)i][j].FullName.Surname.empty() /*&&
                !FIOClusterVectors[(EFIOType)i][j].FullName.FoundSurname */)
                allClusters.push_back(&FIOClusterVectors[(EFIOType)i][j]);
        }

    for( i = 0 ; i < PotentialSurnames.size() ; i++ )
        allClusters.push_back(&PotentialSurnames[i]);

    for( i = 0 ; i < FIOwithSuspiciousPatronymic.size() ; i++ )
        allClusters.push_back(&FIOwithSuspiciousPatronymic[i]);

    for( i = 0 ; i < IOwithSuspiciousPatronymic.size() ; i++ )
    {
        //так как отчества из FIOwithSuspiciousPatronymic тоже могут стать фамилиями (TFioClusterBuilder::ResolvePatronymicInIO),
        //то и этим отчествам (как фамилиям припишем PardigmID)
        IOwithSuspiciousPatronymic[i].FullName.Surname = IOwithSuspiciousPatronymic[i].FullName.Patronymic;
        allClusters.push_back(&IOwithSuspiciousPatronymic[i]);
    }

    sort(allClusters.begin(), allClusters.end(), mem_fn(&TNameCluster::order_by_f_pointer));
    int iCurParadigmID = 0;
    for( i = 0 ; i < allClusters.size() ; i++)
    {
        iCurParadigmID++;
        TFullFIO& fullFio = allClusters[i]->FullName;
        //может оказаться, что этому  ФИО уже приписали
        //на предыдущих итерациях
        if( fullFio.LocalSuranamePardigmID != -1 )
            continue;
        fullFio.LocalSuranamePardigmID = iCurParadigmID;
        if( fullFio.Surname.length() < 2 )
            continue;

        for(size_t j = i+1 ; j < allClusters.size() ; j++ )
        {
            TFullFIO& fullFio2 = allClusters[j]->FullName;
            if( fullFio2.Surname.length() < 2 )
                break;

            //считаем, что если первые две буквы не совпадают, то
            //сравнивать фамилии уже бессмысленно
            if( strncmp(fullFio2.Surname.data(), fullFio.Surname.data(),2) )
                break;

            // genders should be the same
            if (((NSpike::AllGenders & fullFio2.Genders) & (NSpike::AllGenders & fullFio.Genders)).Empty())
                continue;

            if( fullFio2.LocalSuranamePardigmID != -1 )
                continue;

            if( fullFio.FoundSurname != fullFio2.FoundSurname )
                continue;

            if( fullFio.FoundSurname && (fullFio.Surname == fullFio2.Surname) )
                fullFio2.LocalSuranamePardigmID = iCurParadigmID;
            else
                if( !fullFio.FoundSurname && SimilarSurnames(*allClusters[i], *allClusters[j]) )
                    fullFio2.LocalSuranamePardigmID = iCurParadigmID;

        }
    }

    for( i = 0 ; i < IOwithSuspiciousPatronymic.size() ; i++ )
    {
        //очистим фамилии, ранее восстановленные из отчеств
        IOwithSuspiciousPatronymic[i].FullName.Surname.clear();
    }

}

bool TFioClusterBuilder::Run(bool doMerge)
{
    try
    {
        //присвоем PardigmID для всех фамилий (словарных и несловарных),
        //т.е. разобьем все фамилии на классы эквивалентности (для неслованых фамилийи с помощью ф-ции SimilarSurnSing)
        //и пронумеруем эти классы - номер класса и будет LocalSuranamePardigmID
        AssignLocalSurnamePardigmID();

        //проверим, может и нет ничего
        {
            int i = 0;
            for( ; i < FIOTypeCount ; i++ )
            {
                if( FIOClusterVectors[(EFIOType)i].size() > 0 )
                    break;
            }

            if( (i >=  FIOTypeCount) && (IOwithSuspiciousPatronymic.size() == 0)
                && (FIOwithSuspiciousPatronymic.size() == 0) )
                return true;
        }

        for(int i = 0 ; i < FIOTypeCount ; i++ )
            sort(FIOClusterVectors[(EFIOType)i].begin(), FIOClusterVectors[(EFIOType)i].end());

        ResolvePatronymicInIO();
        sort(PotentialSurnames.begin(), PotentialSurnames.end());
        MergeEqual(PotentialSurnames);

        for(int i = 0 ; i < FIOTypeCount ; i++ )
        {
            sort(FIOClusterVectors[(EFIOType)i].begin(), FIOClusterVectors[(EFIOType)i].end());
            MergeEqual(FIOClusterVectors[i]);
        }

        if (!doMerge)
            return true;

        MergeFIandFIin();
        MergeFIOandFIinOin();
        MergeFIandPotentialSurnames();
        MergeFIOandPotentialSurnames();

        MergeFIandF();
        MergeFIOandF();
        MergeFIandI();
        MergeFIOandFI();
        MergeIOandI();
        MergeFIOandI();
        MergeFIOandIO();
        //отрубим пока это стремное слияние для фактов
        //может надо в будущем смотреть на растояние...
        //if( !m_bResolveContradictions )
        //    MergeFIandIO();

        DeleteUsedFIOS();

        for(int i = 0 ; i < FIOTypeCount ; i++ )
        {
            MergeEqual(FIOClusterVectors[i]);
        }

        ResolveSurnameAndFemMuscContradictions();

        if( m_bResolveContradictions )
            //рассматриваем случаи, когда одно фио
            //слилось с двумя противоречивыми фио (А. Петров, Алексей Петров и Алесандр Петров)
            ResolveFirstNameAndPatronymicContradictions();

        return true;
    }

    catch(yexception& e)
    {
        Reset();
        TString msg;
        sprintf( msg, "Error in \"TFioClusterBuilder::Run\":%s", e.what() );
    throw yexception() << msg;
    }
    catch(...)
    {
        Reset();
        ythrow yexception() << "Error in \"TFioClusterBuilder::Run\"";
    }
}

