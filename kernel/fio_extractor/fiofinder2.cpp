#include <library/cpp/charset/codepage.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/strspn.h>
#include <library/cpp/charset/wide.h>
#include <kernel/lemmer/dictlib/grammar_index.h>

#include "fiofinder2.h"
#include "library/cpp/token/nlptypes.h"

using namespace NIndexerCore;

const long TFioFinder2::FirmedNameTemplatesCount = 12;

TFioFinder2::TNameTemplate FirmedNameTemplates2[TFioFinder2::FirmedNameTemplatesCount] =
{
    { {FirstName, MiddleName}, 2, FIOname, true, true},
    { {InitialName, InitialPatronymic}, 2, FIOname, true, true},
    { {FirstName, InitialPatronymic }, 2, FIOname, true, false},
    { {FirstName, InitialName, InitialPatronymic}, 3, FIOname, false, false}, //Мамай В.И.
    { {FirstName, MiddleName}, 2, IOname, false, true},
    { {InitialName, InitialPatronymic}, 2, IOnameInIn, false, true},
    { {FirstName }, 1, FIname, true, true },
    { {InitialName }, 1, IFnameIn, true, true},
    { {FirstName, InitialName}, 2, FInameIn, false, false},    //Мамай В.
    { {FirstName }, 1, Iname, false, true},
    { {Surname}, 1, Fname, false, false},
    { {InitialName }, 1, InameIn, false, true}
};

TFioFinder2::TFioFinder2(const TDirectTextEntry2* entries, size_t count, int forQurey)
    : ForQuery(forQurey)
{
    PriorityToTheRightSurname = true;
    FirstWordInCurSent = 0;
    Cache.resize(count);
    Entries.reserve(count);
    for (size_t i = 0; i < count; ++i)
        Entries.push_back(TTextEntry(entries[i]));
}

void TFioFinder2::AddToSurnames(std::set<TString>& allSurnames, bool bSuspiciousPatronymic, const TFioWordSequence& fioWS) const
{
    if( bSuspiciousPatronymic && fioWS.NameMembers[MiddleName].IsValid() )
    {
        const TTextEntry& entry = Entries[fioWS.NameMembers[MiddleName].WordNum];
        for( int i = 0 ; i < GetFormLemmaInfoCount(entry) ; i++ )
        {
            TString s(::GetLemma(entry, i));
            s = ToLower(s, csYandex);
            allSurnames.insert(s);
        }
    }

    if( fioWS.NameMembers[Surname].IsValid() )
    {
        const TTextEntry& entry = Entries[fioWS.NameMembers[Surname].WordNum];
        for( int i = 0 ; i < GetFormLemmaInfoCount(entry) ; i++ )
        {
            TString s(::GetLemma(entry, i));
            s = ToLower(s, csYandex);
            allSurnames.insert(s);
        }
    }
}

//для вариантов, построенных вокруг одного имени (может с отчеством)
//установим такие границы ФИО, чтобы они были у всех одинаковые и включали все
//построенные варианты. В TFioClusterBuilder мы, может быть, сможем выбрать правильный.
//А пересекающиеся зоны вставлять в индекс нельзя.
static void SetFirstLastWord(TVector< TFioWordSequence >& addedFioVariants)
{
    if(addedFioVariants.ysize() <= 1)
        return;

    int iMaxLastWord = -1;
    int iMinFirstWord = -1;
    for(int i = 0 ; i < addedFioVariants.ysize() ; i++)
    {
        if((iMaxLastWord == -1) || (iMaxLastWord < addedFioVariants[i].LastWord()))
            iMaxLastWord = addedFioVariants[i].LastWord();
        if((iMinFirstWord == -1) || (iMinFirstWord > addedFioVariants[i].FirstWord()))
            iMinFirstWord = addedFioVariants[i].FirstWord();
    }

    if((iMinFirstWord == -1) || (iMaxLastWord == -1) || (iMinFirstWord >= iMaxLastWord))
        return;

    for(int i = 0 ; i < addedFioVariants.ysize() ; i++)
    {
        addedFioVariants[i].SetPair(iMinFirstWord, iMaxLastWord);
    }
}

void TFioFinder2::FindFios(TVector<TFioWordSequence>& foundNames, TVector<int>& potentialSurnames, size_t maxFioCount) const
{
    int curWord = 0;
    int iLastWordInFIO = -1;

    while (curWord < (int)Entries.size()) {

        if (maxFioCount <= foundNames.size())
            break;

        const TTextEntry& entry = Entries[curWord];

        if (!IsUpperForFio(entry) || !CanBeName(entry)) {
            curWord++;
            continue;
        }

        bool bFound = false;

        if( IsName(curWord) )
        {

            bool bAddedOnce = false;
            for( int i = 0 ; i < FirmedNameTemplatesCount ; i++ )
            {
                TVector< TFioWordSequence > foundNameVariants;

                int iW = curWord;

                //так как мы ищем сначала фио без фамилии,
                //а фамилию ищем и слева и справа, а потом выбираем лучшую (при прочих равных - правую)
                //то мы должны пропустить фамлилию, так двигаемся по предложению слева направо
                //но если есть омоним имени, то не пропускаем
                if( IsName(Surname, curWord) && FirmedNameTemplates2[i].CheckSurnameFromDic )
                {
                    int iH = IsName_i(FirstName, curWord);
                    if( iH == -1 )
                        iH = IsName_i(InitialName, curWord);
                    if( iH == -1 )
                        iW++;
                    else
                    {
                        const TTextEntry::TLemmaType& lemmInfo = Entries[curWord].GetFormLemma(iH);
                        if( HasOnlyGrammem(lemmInfo, gPlural) )
                            iW++;
                    }
                }

                if( !FindFirmedName(FirmedNameTemplates2[i], foundNameVariants, iW, iLastWordInFIO) )
                    continue;
                bool bBreak = true;
                TVector< TFioWordSequence > addedFioVariants;
                for(size_t k = 0 ; k < foundNameVariants.size() ; k++ )
                {
                    bool bAdd = true;
                    TFioWordSequence& foundName = foundNameVariants[k];
                    bool bWithSurname = false;
                    //раширяем где нужно несловарными фамилиями фио
                    switch(FirmedNameTemplates2[i].FIOType )
                    {
                        case FIOname:
                        {
                            bWithSurname = true;
                            break;
                        }
                        case IOname:
                        {
                            if( !EnlargeIOname(foundName, iLastWordInFIO) )
                                if( bAddedOnce )
                                    bAdd = false;
                            break;
                        }
                        case FIname:
                        {
                            bWithSurname = true;
                            break;
                        }
                        case Fname:
                        {
                            break;
                        }
                        case Iname:
                        {
                            if( !EnlargeIname(foundName, iLastWordInFIO) )
                                if( bAddedOnce )
                                    bAdd = false;
                            break;
                        }
                        case IOnameIn:
                        {
                            if( !EnlargeIOnameIn(foundName, iLastWordInFIO) )
                            {
                                bBreak = false;
                                bAdd = false;
                                continue;
                            }
                            break;
                        }
                        case IOnameInIn:
                        {
                            EnlargeIOnameInIn(foundName, iLastWordInFIO);
                            if( foundName.size() == 2 )
                                bAdd = false;
                            break;
                        }

                        case IFnameIn:
                        {
                            bWithSurname = true;
                            break;
                        }
                        case InameIn:
                        {
                            EnlargeInameIn(foundName, iLastWordInFIO);
                            if( foundName.size() == 1)
                                bAdd = false;
                            break;
                        }
                        case MWFio:
                        case FInameIn:
                        case FIOTypeCount:
                        case FIOinName:
                        case FIinOinName:
                        case FIinName: break;
                    }

                    if( !bAdd )
                        continue;

                    if( bWithSurname )
                    {
                        if( (foundName.FirstWord() == curWord + 1) && (curWord + 1 == iW) )
                        //для ситуации ...Петрову Александру Иванову жалко ...
                        //Петрову - пропустили (см. выше), анализируя окружение
                        //Александру выбрали Иванову, но и про Петрову не нужно забывать - добавим это как фио
                        //то что перед Петрову нет имени или инициала, гарантирует порядок просмотра предложени слева направо
                            AddSingleSurname(curWord, foundNames);
                    }

                    //NormalizeFio(foundName);
                    if( AddFoundFio(addedFioVariants,foundName) ) {
                        iLastWordInFIO = foundName.LastWord();
                        bAddedOnce = true;
                        curWord = foundName.LastWord();
                        bFound = true;
                    }

                    if( potentialSurnames.size() != 0 )
                        if( potentialSurnames.back() >= foundName.FirstWord() )
                            potentialSurnames.erase(potentialSurnames.begin() + (potentialSurnames.size() - 1));
                }
                ChooseVariants(addedFioVariants, foundNames);
                if( bBreak )
                    break;
            }
        }

        if (!bFound && !IsInitial(entry))
            potentialSurnames.push_back(curWord);

        curWord++;
    }

    //отдельно рассмотрим последовательности двух подряд идущих IName и IOName
    //IName может оказаться фамилией типа  Мамай Павел Александрович
    //TrateNamesAsSurnames(foundNames);
}


//выберем среди конечных вариантов ФИО вокруг одной фамилии те
//у которых фамилия стоит как можно правее,
//например для ...Союза Валерия Новодворская... построится два
//варианта "Союза Валерий" и "Валерия Новодворская", нужно оставить только "Валерия Новодворская"
void TFioFinder2::ChooseVariants(TVector< TFioWordSequence >& addedFioVariants, TVector< TFioWordSequence >& foundNames) const
{
    if( addedFioVariants.size() == 0)
        return;
    if( addedFioVariants.size() == 1)
    {
        foundNames.push_back(addedFioVariants[0]);
        return;
    }

    //если из запроса, то при омонимии муж\жен
    //выбираем ту, которая стоит в им.п.
    //это касается только тех случаев, когда
    //фио стоит в самом начале
    if( ForQuery == 2 )
    {
        std::sort(addedFioVariants.begin(), addedFioVariants.end());
        size_t i;
        for( i = 1 ; i < addedFioVariants.size() ; i++ )
        {
            if( (addedFioVariants[i].FirstWord() > 0) ||
                !addedFioVariants[i].equal_as_pair(addedFioVariants[i-1])  )
                break;
        }
        if( i >= 2 )
        {
            TSet<int> toDel;
            for( int j = (int)i-1; j >= 0; j-- )
            {
                if( !addedFioVariants[j].GetGrammems().Test(gNominative))
                    toDel.insert(j);
            }

            if( toDel.size() < addedFioVariants.size() )
                for( int j = (int)i-1; j >= 0; j-- )
                    if( toDel.find(j) != toDel.end() )
                        addedFioVariants.erase(addedFioVariants.begin()+j);
        }


        int iMaxRightWord = 0;
        TVector<bool> gleichedFios;
        gleichedFios.resize(addedFioVariants.size(), false);
        bool bAllNotGleiched = true;
        i = 0 ;

//        bool bEqualFioPos = true;

        for(; i < addedFioVariants.size() ; i++ )
        {
            TFioWordSequence& oc = addedFioVariants[i];
            if( oc.NameMembers[Surname].IsValid() )
            {
                int iW = oc.NameMembers[Surname].WordNum;
//                if( (iMaxRightWord != iW) && (i > 0) )
//                    bEqualFioPos = false;
                if( iMaxRightWord < iW)
                    iMaxRightWord = iW;
                if( oc.NameMembers[FirstName].IsValid() )
                {
                    gleichedFios[i] = true;//m_pMorph->GleicheGendreNumberCaseStandart(m_Words[oc.NameMembers[Surname]], m_Words[oc.NameMembers[FirstName]]) != 0;
                    if( gleichedFios[i] )
                        bAllNotGleiched = false;
                }
            }
        }

        for(size_t i2 = 0 ; i2 < addedFioVariants.size() ; i2++ )
        {
            TFioWordSequence& oc = addedFioVariants[i2];
            if( oc.NameMembers[Surname].IsValid() /*&& !bEqualFioPos*/ )
            {
                int iW = oc.NameMembers[Surname].WordNum;
                if( iMaxRightWord == iW && (bAllNotGleiched ||
                                            gleichedFios[i2]) )
                    foundNames.push_back(addedFioVariants[i2]);
            }
            else
            {
                foundNames.push_back(addedFioVariants[i2]);
            }
        }
    }
    else
    {
        SetFirstLastWord(addedFioVariants);
        foundNames.insert(foundNames.end(),addedFioVariants.begin(), addedFioVariants.end());
    }
}

//разбиремся с отчеством в запросе, считаем, что
//все отчества в хапросе можно считать фамилиями, за исключением тех случаев, когда
//запрос состоит не только из имени отчества
void TFioFinder2::ChangePatronymicToSurnameForQuery(TFioWordSequence& foundName) const
{
    if( foundName.NameMembers[Surname].IsValid() )
        return;
    if( !foundName.NameMembers[MiddleName].IsValid() || !foundName.NameMembers[FirstName].IsValid() )
        return;
    TWordHomonymNum wh = foundName.NameMembers[MiddleName];
    bool bReplace = false;
    if (IsBastard(Entries[wh.WordNum]))
    {
        bReplace = true;
    } else {
        int iH = IsName_i(Surname, wh.WordNum, false);
        if( iH != -1 )
        {
            bReplace = true;
            wh.HomNum = iH;
        }
        //if( AllEntries.Count == 2 )
        //{
        //    bReplace = true;
        //    break;
        //}
    }

    if( bReplace )
    {
        foundName.NameMembers[Surname] = wh;
        foundName.Fio.Surname = foundName.Fio.Patronymic;
        foundName.Fio.Patronymic.clear();
        foundName.Fio.FoundSurname = true;
        foundName.NameMembers[MiddleName].Reset();
    }
}

bool TFioFinder2::AddFoundFio(TVector< TFioWordSequence >& foundNames, TFioWordSequence& foundName) const
{

    //не все способы выделения фио проходят, если мы анализируем запрос
    //(так как мы не обращаем внимание на большую букву и на точку после инициалов)
    if( ForQuery )
    {
        if( foundName.size() == 1 )
            return false;

        ChangePatronymicToSurnameForQuery(foundName);

        if( !foundName.NameMembers[Surname].IsValid() )
            return false;

        if( (foundName.NameMembers[FirstName].WordNum == 0) ||
            (   (foundName.NameMembers[FirstName].WordNum == 1) &&
                (foundName.NameMembers[Surname].WordNum == 0)
            ) && ForQuery == 2 )
        {
            const TTextEntry& firstNameEntry = Entries[foundName.NameMembers[FirstName].WordNum];
            const TTextEntry::TLemmaType& firstName = firstNameEntry.GetFormLemma(foundName.NameMembers[FirstName].HomNum);
            if( !HasOnlyGrammem(firstName, gNominative) &&
                !HasOnlyGrammem(firstNameEntry, gFirstName) )
            {
                return false;
            }

        }
    }

    if (!!Filter && (*Filter)(*this, foundName))
        return false;

    foundName.GetFio().ToLower();
    foundNames.push_back(foundName);
    return true;

}

void TFioFinder2::AddSingleSurname(int iW, TVector< TFioWordSequence >& foundNames) const
{
    int iH = IsName_i(Surname, iW);
    const TTextEntry& entry = Entries[iW];
    if( (iH == -1) || IsBastard(entry) || !IsUpperForFio(entry) )
        return;
    TFioWordSequence foundName;// = make_pair(SFullFIO(m_pMorph), CFIOOccurence());
    foundName.SetPair(iW, iW);
    foundName.NameMembers[Surname] = TWordHomonymNum(iW, iH);
    //foundName.Fio.m_strSurname = m_Words[iW].GetHomonym(iH)->GetLemma();
    foundName.Fio.FoundSurname = true;
    TVector< TFioWordSequence > nameVariants;
    CheckGrammInfo(foundName, nameVariants);
    for( size_t i = 0 ; i < nameVariants.size() ; i++ )
    {
        //NormalizeFio(nameVariants[i]);
        AddFoundFio(foundNames, nameVariants[i]);
    }
}

//ищем несловарную фамилию для двух инициалов
bool TFioFinder2::EnlargeIOnameInIn(TFioWordSequence& foundName, int lastWordInFIO) const
{
    if( !foundName.NameMembers[InitialName].IsValid() || !foundName.NameMembers[InitialPatronymic].IsValid())
        return false;
    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    return EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord);
}

//ищем несловарную фамилию для имени и инициала
bool TFioFinder2::EnlargeIOnameIn(TFioWordSequence& foundName, int ) const
{
    if( !foundName.NameMembers[FirstName].IsValid() || !foundName.NameMembers[InitialPatronymic].IsValid())
        return false;

    int iWName = foundName.NameMembers[FirstName].WordNum;
    if( iWName == -1)
        iWName = foundName.NameMembers[InitialName].WordNum;


    int nextWord = foundName.LastWord() + 1;
    if (nextWord < (int)Entries.size())
        if( CanBeSurnameFromTheRight(nextWord, iWName) &&
            CanBeLinked(Entries[nextWord - 1], Entries[nextWord]))
        {
            const TTextEntry& entry = Entries[nextWord];
            foundName.ChangeLastWord(nextWord);
            foundName.NameMembers[Surname] = TWordHomonymNum(nextWord, 0);
            foundName.Fio.Surname = ::GetFormText(entry);
            return true;
        }

    return false;
}

//ищем несловарную фамилию для одиночного инициала
bool TFioFinder2::EnlargeInameIn(TFioWordSequence& foundName, int lastWordInFIO) const
{
    if( !foundName.NameMembers[InitialName].IsValid() )
        return false;

    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    return EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord);
}


bool TFioFinder2::EnlargeIname(TFioWordSequence& foundName, int lastWordInFIO) const
{
    if( !foundName.NameMembers[FirstName].IsValid() )
        return false;
    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    return EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord);
}

//ищем несловарную фамилию для имени и отчества
bool TFioFinder2::EnlargeIOname(TFioWordSequence& foundName, int lastWordInFIO) const
{
    if( !foundName.NameMembers[FirstName].IsValid() || !foundName.NameMembers[MiddleName].IsValid() )
        return false;

    int nextWord = foundName.LastWord() + 1;
    int prevWord = foundName.FirstWord() - 1;

    if( EnlargeBySurname(foundName, lastWordInFIO, nextWord, prevWord) )
        return true;

    return false;
}

bool TFioFinder2::CanBeSurnameFromTheLeft(int iW, bool bAtTheEndOfSent, TFioWordSequence& foundName) const
{
    const TTextEntry& entry = Entries[iW];
    if( !CanBeName(entry) )
        return false;

    if( ForQuery && HasTwoUpper(entry) )
        return false;

    bool bCheckIsBastard = (ForQuery > 0) &&
                            //разрешим быть словарным словом, если фио совпадает с запросом и есть очество  и фамилия с большой буквы
                            !(foundName.NameMembers[MiddleName].IsValid() && (iW == 0) && bAtTheEndOfSent && ::IsUpper(Entries[iW]));

    return  IsUpperForFio(entry) &&
            (!IsName(FirstName, iW) || bAtTheEndOfSent) &&
            !IsName(Surname, iW) &&
            (!bCheckIsBastard || IsBastard(entry) ) &&
            !IsInitial(entry) &&
            (    IsBastard(entry) ||
                (/*!IsBegOfSentence(iW, AllEntries) &&  */HasOnlyPOSes(entry, TGramBitSet(gSubstantive, gAdjective) ) ) ||
                !PriorityToTheRightSurname  ||
                ( bAtTheEndOfSent && IsBegOfSentence(iW) && ( HasOnlyPOSes(entry, TGramBitSet(gSubstantive, gAdjective) ) ||
                                                                          (ForQuery > 0) ) )
             ) &&
            (strlen(::GetFormText(entry).data()) > 1);
}

bool TFioFinder2::CanBeSurnameFromTheRight(int iW, int /*iWName*/) const
{
    const TTextEntry& entry = Entries[iW];
    if( !CanBeName(entry) )
        return false;

    if( ForQuery && HasTwoUpper(entry) )
        return false;

    bool bCheckIsBastard = false;
    //if( (ForQuery > 0) && iWName != -1 )
    //{
    //    //if( !HasOnlyGrammem(AllEntries.Entries[iWName], gFirstName) )
 //       if( !HasOnlyGrammems(AllEntries.Entries[iWName], NSpike::MakeMask(gFirstName) | NSpike::MakeMask(gSurname)) )
    //        bCheckIsBastard = true;
    //}

    return      IsUpperForFio(entry)&&
                !IsName(Surname, iW, false) &&
                !IsName(FirstName, iW, false)&&
                !IsInitial(entry) &&
                (!bCheckIsBastard || IsBastard(entry) ) &&
                (strlen(::GetFormText(entry).data()) > 1);
}

bool TFioFinder2::NameCanBeSurnameFromTheRight( TFioWordSequence& foundName, int iW, int& iSurnameHFromRight) const
{
    const TTextEntry& entry = Entries[iW];
    if( !IsUpperForFio(entry) )
        return false;
    TVector<int> firstNameHomonyms;
    GetNameHomonyms(FirstName, entry, firstNameHomonyms);
    if( firstNameHomonyms.size() == 0 )
        return false;
    if( !IsEndOfSentence(iW) )
    {

        int iWName = foundName.NameMembers[FirstName].WordNum;
        if( iWName == -1)
            iWName = foundName.NameMembers[InitialName].WordNum;

        int nextEntryNum = iW + 1;
        const TTextEntry& nextEntry = Entries[nextEntryNum];
        if( IsName(Surname, nextEntryNum) ||
            IsName(MiddleName, nextEntryNum) )
            return false;
        if( CanBeSurnameFromTheRight(nextEntryNum, iW) &&
            CanBeLinked(entry, nextEntry))
            return false;
    }

    int iFirstNameH = -1;
    if( foundName.NameMembers[FirstName].IsValid() )
    {
        size_t i;
        for( i = 0 ; i < firstNameHomonyms.size() ; i++ )
        {
            iFirstNameH = firstNameHomonyms[i];
            TWordHomonymNum& firstname_ind = foundName.NameMembers[FirstName];
            const TTextEntry::TLemmaType& firstname1 = Entries[firstname_ind.WordNum].GetFormLemma(firstname_ind.HomNum);
            const TTextEntry::TLemmaType& firstname2 = Entries[iW].GetFormLemma(iFirstNameH);
            if( GleicheCaseNumber(firstname1, firstname2).any() )
                break;
        }
        if( i >= firstNameHomonyms.size() )
            return false;
    }
    else
        iFirstNameH = firstNameHomonyms[0];

    iSurnameHFromRight = iFirstNameH;
    return true;
}

//согласование для предсказанной фамилии
//(предсказывается хреново, так что пока все время говорим, что согласованы)
bool TFioFinder2::GleichePredictedSurname(TFioWordSequence& , int , int& , TGramBitSet& ) const
{
    return true;
//    if( !IsNamPredictSurname(foundName.second, iW) )
//        return true;
//    TWordHomonymNum whFirstName = foundName.second.NameMembers[FirstName];
//    CWordRus& wSurname = m_Words.GetOriginalWord(iW);
//
//    bool bRes = false;
//    CWordRus::SHomIt it = wSurname.GetHomonymIterBegin();
//    //предсказанных фамилий может быть несколько
//    //например, Шиховой - как Шихов или как Шиховой
//    //ищем согласованные с именем
//    for( ; !it.IsEnd() ; it++ )
//    {
//        const CHomonymRus& hSurname = *it;
//        if( !hSurname.HasGrammem(SurName) )
//            continue;
//
//        newH = it.GetID();
//        if( whFirstName.IsValid() )
//        {
//            const CHomonymRus& hFirstName = m_Words[whFirstName];
//            QWORD commonGrammems = m_pMorph->GleicheGendreNumberCaseStandart(hSurname, hFirstName);
//            if( commonGrammems != 0 )
//            {
//                bRes = true;
//                break;
//            }
//        }
//        else
//        {
//            commonGrammems = hSurname.GetGrammemsWhichContain( (foundName.first.m_Genders & AllGenders)|NSpike::MakeMask(Singular));
//            if(  commonGrammems != 0 )
//            {
//                bRes = true;
//                break;
//            }
//        }
//    }
//
//    //если согласование не прошло, а имя неизменяемое или фамилия несловарное слово, то считаем, что имя иностранное,
//    //а предсказание случайное, херим все предсказанные омонимы
//    //и говорим, что все зашибись (типа Катрин Денев)
//    if( !bRes )
//    {
//        if( whFirstName.IsValid() )
//            if( m_Words[whFirstName].HasGrammems(AllCases) ||
//                !wSurname.IsFoundInMorph() )
//            {
//                wSurname.DeleteHomonymWithPOSAndGrammems(Noun, NSpike::MakeMask(SurName), true );
//                if( wSurname.GetRusHomonymsCount() == 0 )
//                    wSurname.CreateHomonyms(false);
//                newH = -1;
//                return true;
//            }
//    }
//
//    return bRes;
}

bool TFioFinder2::EnlargeBySurname(TFioWordSequence& foundName, int , int nextWord, int prevWord) const
{
    bool bFromRight = false;
    bool bFromLeft = false;

    int iSurnameHFromRight = -1;
    int iSurnameHFromLeft = -1;

    TGramBitSet iCommonGrammemsFromRight;
    TGramBitSet iCommonGrammemsFromLeft;

    int iWName = foundName.NameMembers[FirstName].WordNum;
    if( iWName == -1)
        iWName = foundName.NameMembers[InitialName].WordNum;

    if (!IsInNextSent(nextWord)) {
        const TTextEntry& prevEntry = Entries[nextWord - 1];
        const TTextEntry& thisEntry = Entries[nextWord];
        if (!HasCloseQuote(prevEntry))
            if( CanBeSurnameFromTheRight(nextWord, iWName) &&
                GleichePredictedSurname(foundName, nextWord, iSurnameHFromRight, iCommonGrammemsFromRight) &&
                CanBeLinked(prevEntry, thisEntry))
                bFromRight = true;
            else
                if( PriorityToTheRightSurname &&
                    NameCanBeSurnameFromTheRight(foundName, nextWord, iSurnameHFromRight) &&
                    CanBeLinked(prevEntry, thisEntry))
                    bFromRight = true;
    }

    if (!IsInPrevSent(prevWord)) {
        const TTextEntry& thisEntry = Entries[prevWord];
        const TTextEntry& nextEntry = Entries[prevWord + 1];
        if (!NextHasOpenQuote(thisEntry)) {
            bool bAtTheEndOfSentence = (foundName.LastWord() == (int)Entries.size() - 1);
            if( !bAtTheEndOfSentence ) {
                const TTextEntry& e = Entries[foundName.LastWord() + 1];
                bAtTheEndOfSentence = HasPunct(e) ||
                                      HasGrammem(e, gConjunction) ||
                                      IsInDifferentSentences(foundName.LastWord(), foundName.LastWord() + 1, &Entries[0], Entries.size());
            }
            if( CanBeSurnameFromTheLeft(prevWord, bAtTheEndOfSentence, foundName) &&
                GleichePredictedSurname(foundName, prevWord, iSurnameHFromLeft, iCommonGrammemsFromLeft) &&
                CanBeLinked(thisEntry, nextEntry))
            {
                bFromLeft = true;
            }

            if(!bFromLeft &&
               SurnameCanBeLinkedFromLeftThroughComma(prevWord) &&
               IsUpperForFio(thisEntry) &&
               foundName.NameMembers[FirstName].IsValidWordNum() &&
               foundName.NameMembers[MiddleName].IsValidWordNum() &&
               CanBeEndOfCommaSeparatedFio(foundName.LastWord() ) )
            {
                bFromLeft = true;
            }
        }
    }

    bool bPriorityToTheRightSurname = PriorityToTheRightSurname;
    if( (iCommonGrammemsFromRight.any()) && (iCommonGrammemsFromLeft.none()) )
        bPriorityToTheRightSurname = true;

    if( (iCommonGrammemsFromRight.none()) && (iCommonGrammemsFromLeft.any()) )
        bPriorityToTheRightSurname = false;

    if(bFromRight && bFromLeft)
    {
        int iWSCountFromLeft = GetWSCount(prevWord);
        int iWSCountFromRight = GetWSCount(nextWord-1);
        if((iWSCountFromLeft <= 1) && (iWSCountFromRight > 1))
            bFromRight = false;
        else
            if((iWSCountFromRight <= 1) && (iWSCountFromLeft > 1))
                bFromLeft = false;
    }

    if(bFromRight && bFromLeft && bPriorityToTheRightSurname &&
       IsBegOfSentence(prevWord))
    {
        bFromLeft = false;
    }

    if( bFromRight && (bPriorityToTheRightSurname || !bFromLeft) )
    {
        const TTextEntry& surnameEntry = Entries[nextWord];
        foundName.ChangeLastWord(nextWord);
        foundName.NameMembers[Surname] = TWordHomonymNum(nextWord, 0); // it is not assigned!
        foundName.Fio.Surname = ::GetFormText(surnameEntry);
        if( iCommonGrammemsFromRight.any() )
            foundName.PutGrammems(iCommonGrammemsFromRight);
        return true;
    }

    if( bFromLeft && (!bPriorityToTheRightSurname || !bFromRight) )
    {
        const TTextEntry& surnameEntry = Entries[prevWord];
        foundName.ChangeFirstWord(prevWord);
        foundName.NameMembers[Surname] = TWordHomonymNum(prevWord, 0);
        foundName.Fio.Surname = ::GetFormText(surnameEntry);
        if( iCommonGrammemsFromLeft.any() )
            foundName.PutGrammems(iCommonGrammemsFromLeft);
        return true;
    }

    return false;
}

inline bool IsLatin(wchar16 ch) {
    return 'a' <= ch && ch <= 'z' || 'A' <= ch && ch <= 'Z';
}

inline bool IsInitialLatin(const char* form) {
    Y_ASSERT(strlen(form) < 3 && strlen(form) > 0);
    wchar16 buf[3];
    ConvertKeyText(form, buf);
    return IsLatin(buf[0]) || buf[0] && IsLatin(buf[1]);
}

bool TFioFinder2::IsInitial(const TTextEntry& entry) const {
    if (entry.GetFormCount() != 1)
        return false;
    const char* form = entry.GetForm(0).FormaText;
    {
        if( form && (strlen(form) <= 2) && (strlen(form) >= 1) &&
            (entry.GetSpaceCount() >= 1) && (entry.GetSpace(0).Length >= 1) &&
            !IsInitialLatin(form) &&
            (WideCharToYandex.Tr(entry.GetSpace(0).Space[0]) == '.') )
            return true;
    }
    return false;
}

void TFioFinder2::FillWordFioInfo(size_t iEntry) const
{
    SWordFioInfo& wordFioInfo = Cache[iEntry];
    const TTextEntry& entry = Entries[iEntry];

    if( !CanBeName(entry) )
    {
        wordFioInfo.Checked = true;
        return;
    }

    if( !IsUpperForFio(entry) )
        return;

    size_t lemmCount = entry.GetFormLemmaCount();
    wordFioInfo.HomonymFioFlags.resize(lemmCount, 0);

    if( IsInitial(entry) )
    {
        wordFioInfo.FioFlags |= (1 << InitialName);
        wordFioInfo.FioFlags |= (1 << InitialPatronymic);
        for( size_t i = 0 ; i <  lemmCount ; i++ )
            wordFioInfo.HomonymFioFlags[i] = wordFioInfo.FioFlags;
        wordFioInfo.Checked = true;
        return;
    }

    for( size_t i = 0 ; i <  lemmCount ; i++ )
    {
        const TTextEntry::TLemmaType& lemmInfo = entry.GetFormLemma(i);

        if( IsName(FirstName,lemmInfo, true) && !HasOnlyGrammem(lemmInfo, gPlural) && CanBeFirstName(entry, lemmInfo) )
        {
            wordFioInfo.FioFlags |= (1 << FirstName);
            wordFioInfo.HomonymFioFlags[i] |= (1 << FirstName);
        }

        if( IsName(Surname,lemmInfo, true) )
        {
            wordFioInfo.FioFlags |= (1 << Surname);
            wordFioInfo.HomonymFioFlags[i] |= (1 << Surname);
        }

        if( IsName(MiddleName,lemmInfo, true) )
        {
            wordFioInfo.FioFlags |= (1 << MiddleName);
            wordFioInfo.HomonymFioFlags[i] |= (1 << MiddleName);
        }
    }
    wordFioInfo.Checked = true;
}

bool TFioFinder2::CanBeFirstName(const TTextEntry& entry, const TTextEntry::TLemmaType& lemmInfo) const
{
    if( entry.GetFormCount() > 1 )
        {
            const char* strLemma = lemmInfo.LemmaText;
            if( strcspn(strLemma, ".-'") == strlen(strLemma) )
                return false;
        }
    return true;
}

bool TFioFinder2::IsName(size_t iEntry) const
{
    Y_ASSERT(iEntry < Entries.size() );
    if( !Cache[iEntry].Checked )
        FillWordFioInfo(iEntry);

    return Cache[iEntry].FioFlags != 0;
}


bool TFioFinder2::IsName(ENameType type, size_t iEntry, bool canBeBastard) const
{
    Y_ASSERT(iEntry < Cache.size() );
    if( !Cache[iEntry].Checked )
        FillWordFioInfo(iEntry);

    if( !canBeBastard )
        if (IsBastard(Entries[iEntry]))
            return false;

    return (Cache[iEntry].FioFlags & (1 << type)) != 0;

}

bool TFioFinder2::IsName(ENameType type, size_t iEntry, size_t iHom, bool canBeBastard /*= true*/) const
{
    Y_ASSERT(iEntry < Cache.size() );
    if( !Cache[iEntry].Checked )
        FillWordFioInfo(iEntry);

    if( !canBeBastard )
        if (GetFormLemmaInfo(iEntry, iHom).IsBastard)
            return false;

    return (Cache[iEntry].HomonymFioFlags[iHom] & (1 << type)) != 0;

}

int TFioFinder2::IsName_i(ENameType type, size_t iEntry, bool canBeBastard /*= true*/) const
{
    Y_ASSERT(iEntry < Cache.size() );
    if( !Cache[iEntry].Checked )
        FillWordFioInfo(iEntry);

    if( (Cache[iEntry].FioFlags & (1 << type)) == 0)
        return -1;

    if( !canBeBastard )
        if (IsBastard(Entries[iEntry]))
            return false;


    for( size_t i = 0 ; i < Cache[iEntry].HomonymFioFlags.size() ; i++ )
        if( Cache[iEntry].HomonymFioFlags[i] & (1 << type))
            return i;

    return -1;
}

bool TFioFinder2::GetNameHomonyms(ENameType type, const TTextEntry& entry, TVector<int>& homs) const
{
    if( (type == InitialName) ||
        (type == InitialPatronymic) )
    {
        if( IsInitial(entry) )
        {
            homs.push_back(0);
            return true;
        }
        else
            return false;
    }

    for (int i = 0; i < (int)entry.GetFormLemmaCount(); ++i) {
        const TTextEntry::TLemmaType& lemmInfo = entry.GetFormLemma(i);
        if( IsName(type,lemmInfo ))
            homs.push_back(i);
    }
    return homs.size() > 0;
}

//проверяем, может ли отчество быть фамилией
bool TFioFinder2::HasSuspiciousPatronymic(TFioWordSequence& fioWS) const
{
    if( !fioWS.NameMembers[MiddleName].IsValid() )
        return false;

    if( fioWS.NameMembers[Surname].IsValid() )
        return false;


    int wPatronymic = fioWS.NameMembers[MiddleName].WordNum;
    const TTextEntry& Patronymic = Entries[wPatronymic];

    //если отчество предсказано или оно также может быть словарной фамилией,
    //то это подозрительно
    if( !IsBastard(Patronymic) && !IsName(Surname, wPatronymic) )
        return false;

    if( fioWS.NameMembers[Surname].IsValid() )
    {
        TWordHomonymNum whSurname = fioWS.NameMembers[Surname];
         //если отчество перед фамилией, то это уж точно отчество
        if( wPatronymic < whSurname.WordNum )
            return false;

        //если фамилия словарная, то независимо от места, считаем, что
        //отчество вне подозрений
        if( IsName(Surname, whSurname.WordNum) )
            return false;
    }

    if( IsBastard(Patronymic) )
        fioWS.Fio.FoundPatronymic = false;

    return true;
}

bool TFioFinder2::IsName(ENameType type, const TTextEntry::TLemmaType& lemmInfo, bool canBeBastard ) const {

    bool bRes = ::IsName(type, canBeBastard, lemmInfo.StemGram, lemmInfo.IsBastard);
    if( !bRes )
        return false;

    if( lemmInfo.IsBastard && (strlen(lemmInfo.LemmaText) <= 3))
        return false;

    return bRes;
}

static const TCompactStrSpn S_VOWELS(
    "\xf3\xe5\xfb\xe0\xee\xfd\xff\xe8\xfe\xb8"
    "\xd3\xc5\xdb\xc0\xce\xdd\xdf\xc8\xde\xa8"
);

bool TFioFinder2::CanBeName(const TTextEntry& entry) const
{
    const size_t n = entry.GetFormCount();
    if (!n)
        return false;

    bool hasVowel = false;
    for (size_t i = 0; i < n; ++i) {
        const TTextEntry::TFormType& token = entry.GetForm(i);
        if (!token.FormaText)
            return false;
        const char* form = token.FormaText;
        size_t len = strlen(form);

        if (GuessTypeByWord(form, len) != NLP_WORD)
            return false;

        if (len == 1 || S_VOWELS.FindFirstOf(form) != form + len)
            hasVowel = true;

        if (token.Lang != LANG_RUS) {
            if (*form == 127)
                return false;
            wchar16* buf = (wchar16*) alloca(sizeof(wchar16) * (len + 1));
            CharToWide(form, len, buf, csYandex);
            if (!NLemmer::ClassifyLanguage(buf, len).Test(LANG_RUS))
                return false;
        }
    }

    return hasVowel;
}

bool TFioFinder2::IsUpperForFio(const TTextEntry& entry) const {
    if( ForQuery )
        return true;
    return ::IsUpper(entry);
}

bool TFioFinder2::HasPunct(const TTextEntry& entry) const {
    for (size_t i = 0; i < entry.GetSpaceCount(); ++i) {
        for (size_t j = 0; j < entry.GetSpace(i).Length; ++j)
            if (strchr(",.-!?:;()[]", WideCharToYandex.Tr(entry.GetSpace(i).Space[j])))
                return true;
    }
    return false;
}

bool TFioFinder2::NextHasOpenQuote(const TTextEntry& entry) const {
    int iLastSp1 = entry.GetSpaceCount() - 1;
    if( iLastSp1 < 0 )
        return false;

    int iLastSp2 = entry.GetSpace(iLastSp1).Length - 1;
    if( iLastSp2 < 0 )
        return false;

    int iPrevSp1 = iLastSp1;
    int iPrevSp2 = iLastSp2-1;
    if( iPrevSp2 < 0 )
    {
        iPrevSp1 = iLastSp1-1;
        if( iPrevSp1 < 0 )
            return false;
        iPrevSp2 = entry.GetSpace(iPrevSp1).Length - 1;
        if( iPrevSp2 < 0 )
            return false;
    }

    if (((WideCharToYandex.Tr(entry.GetSpace(iLastSp1).Space[iLastSp2]) == '\"') || (WideCharToYandex.Tr(entry.GetSpace(iLastSp1).Space[iLastSp2]) == '\'')) &&
        (WideCharToYandex.Tr(entry.GetSpace(iPrevSp1).Space[iPrevSp2]) == ' ') )
        return true;

    return false;
}

bool TFioFinder2::HasCloseQuote(const TTextEntry& entry) const {
    if (entry.GetSpaceCount() >= 1)
        if ((entry.GetSpace(0).Length >= 1) &&
            (WideCharToYandex.Tr(entry.GetSpace(0).Space[0]) == '\"') || (WideCharToYandex.Tr(entry.GetSpace(0).Space[0]) == '\''))
            return true;

    return false;
}

bool TFioFinder2::CanBeEndOfCommaSeparatedFio(int iW) const
{
    if (IsEndOfSentence(iW))
        return true;
    if (HasPunct(Entries[iW]))
        return true;
    if (iW + 1 < (int)Entries.size()) {
        TString s = ::GetFormText(Entries[iW + 1]);
        if( !s.empty() && csYandex.IsDigit((unsigned char)s[0]) )
            return true;
    }
    return false;
}

//нет ли между словами знаков препинания и границы предложения
bool TFioFinder2::CanBeLinked(const TTextEntry& entry1, const TTextEntry& entry2) const {
    if (TWordPosition::Break((i32)entry1.GetPosting()) != TWordPosition::Break((i32)entry2.GetPosting()))
        return false;

    size_t j = IsInitial(entry1) ? 1 : 0;
    for (size_t i = 0; i < entry1.GetSpaceCount(); ++i) {
        for (; j < entry1.GetSpace(i).Length ; ++j) {
            unsigned char c = WideCharToYandex.Tr(entry1.GetSpace(i).Space[j]);
            if( ( c != ' ' )  && ( c != '\r' ) && ( c != '\n' ) && ( c != 0xA0 ) )
                return false;
        }
        j = 0;
    }
    return true;
}

int TFioFinder2::GetWSCount(int iW) const
{
    const TTextEntry& entry = Entries[iW];
    size_t j = (IsInitial(entry) ? 1 : 0);
    int count = 0;
    for (size_t i = 0 ; i < entry.GetSpaceCount(); ++i) {
        const TDirectTextSpace& space = entry.GetSpace(i);
        for (; j < space.Length; ++j) {
            unsigned char c = WideCharToYandex.Tr(space.Space[j]);
            if( ( c != ' ' )  && ( c != '\r' ) && ( c != '\n' ) && ( c != 0xA0 ) )
                return 0;
            else
            {
                count++;
                //if(c == '\n') //считаем \n за 2 пробела, типа это более жесткий разделитель, чем просто пробел
                //    count++;
            }
        }
        j = 0;
    }
    return count;
}

//между словами может быть только запятая (но слова должны быть в одном предложении)
bool TFioFinder2::SurnameCanBeLinkedFromLeftThroughComma(int iSurname) const
{
    const TTextEntry& entry1 = Entries[iSurname];
    const TTextEntry& entry2 = Entries[iSurname + 1];

    if (TWordPosition::Break((i32)entry1.GetPosting()) != TWordPosition::Break((i32)entry2.GetPosting()))
        return false;

    //в запросах разрешаем ФИО написанные только в одинаковом регистре
    if (ForQuery && (::IsUpper(entry1) != ::IsUpper(entry2)))
        return false;

    size_t j = 0;
    if (IsInitial(entry1))
        return false;

    bool hasComma = false;
    for (size_t i = 0; i < entry1.GetSpaceCount(); ++i) {
        const TDirectTextSpace& space = entry1.GetSpace(i);
        for (; j < space.Length; ++j) {
            unsigned char c = WideCharToYandex.Tr(space.Space[j]);
            if( ( c != ' ' )  && ( c != '\r' ) && ( c != '\n' ) && ( c != 0xA0 ) && (c != ',') )
                return false;
            if(c == ',')
                hasComma = true;
        }
        j = 0;
    }

    if(!hasComma)
        return false;

    //должно быть в начале предложения
    if (IsBegOfSentence(iSurname))
        return true;

        //TString s = GetFormText(AllEntries.Entries[iW+1]);
        //if( !s.empty() && csYandex.IsDigit((unsigned char)s[0]) )
        //    return true;


    //такой хак, для случаев типа "6. Гиппиус, Зинаида Николаевна - ..."
    //цифры с точкой не являются концом предложения, поэтому разрешим данный конкретный случай,
    //когда фамилия, отделенная запятой, стоит после числа с точкой
    if( iSurname > 0 )
    {
        TString sPrevForm = ::GetFormText(Entries[iSurname - 1]);
        if( !sPrevForm.empty() && (strspn(sPrevForm.data(), "1234567890") == sPrevForm.length()) )
        {
            const TTextEntry& prevDig = Entries[iSurname - 1];
            for (size_t i = 0; i < prevDig.GetSpaceCount(); ++i) {
                const TDirectTextSpace& space = prevDig.GetSpace(i);
                for (size_t j2 = 0 ; j2 < space.Length; ++j2)
                    if (strchr(".-:)", WideCharToYandex.Tr(space.Space[j2])))
                        return true;
            }

        }

        const TTextEntry& prevWord = Entries[iSurname - 1];
        for (size_t i = 0; i < prevWord.GetSpaceCount(); ++i) {
            const TDirectTextSpace& space = prevWord.GetSpace(i);
            for (size_t j2 = 0; j2 < space.Length ; j2++ ) {
                ui64 mask = (ULL(1)<<(Po_OTHER));// Po_OTHER : '*' [#%&*/@\] NUMBER SIGN ... REVERSE SOLIDUS
                if (NUnicode::CharHasType(csYandex.unicode[(unsigned char)space.Space[j2]], mask))
                    return true;
            }
        }
    }

    return false;
}

//проверяет согласование имени и word_ind (фамилии или отчества)
TGramBitSet TFioFinder2::FirstNameAgreement(const TWordHomonymNum& word_ind, const TWordHomonymNum& first_name_ind) const
{
    const TTextEntry::TLemmaType& word = GetFormLemmaInfo(word_ind);
    const TTextEntry::TLemmaType& firstName = GetFormLemmaInfo(first_name_ind);
    TGramBitSet grammems = GleicheGenderNumberCase(word, firstName);
    if( grammems.Test(gPlural) &&
        !grammems.Test(gSingular))
        return TGramBitSet();
    return grammems;
}

bool TFioFinder2::CheckAgreements(TFioWordSequence& foundName) const
{
    TGramBitSet commonFirstNameSurnameGrammems, resGrammems;

    //согласование между именем и фамилией
    if( foundName.NameMembers[Surname].IsValid() &&
        foundName.NameMembers[FirstName].IsValid() )
    {
        commonFirstNameSurnameGrammems = FirstNameAgreement(foundName.NameMembers[Surname], foundName.NameMembers[FirstName]);
        resGrammems = commonFirstNameSurnameGrammems;
        if( commonFirstNameSurnameGrammems.none() )
            return false;
    }

    TGramBitSet commonPatronymicSurnameGrammems;

    //согласование между именем и отчеством
    if( foundName.NameMembers[MiddleName].IsValid() &&
        foundName.NameMembers[FirstName].IsValid() )
    {
        commonPatronymicSurnameGrammems = FirstNameAgreement(foundName.NameMembers[MiddleName], foundName.NameMembers[FirstName]);
        resGrammems = commonPatronymicSurnameGrammems;
        if( commonPatronymicSurnameGrammems.none() )
            return false;
    }

    //пересечение граммем (именем и фамилией) и (именем и отчеством)
    if( (commonPatronymicSurnameGrammems.any() ) && (commonFirstNameSurnameGrammems.any() ) )
    {
        //TString s1 = GrammemsSet2Str(commonFirstNameSurnameGrammems);
        //TString s2 = GrammemsSet2Str(commonPatronymicSurnameGrammems);
        resGrammems = commonFirstNameSurnameGrammems & commonPatronymicSurnameGrammems & (NSpike::AllCases | NSpike::AllGenders | NSpike::AllNumbers);
        if( resGrammems.none() )
            return false;
    }

    //возьмем нужные граммемы от фамилии, когда либо вообще нет имени, либо только инициал, с которого ничего не возьмешь
    if( foundName.NameMembers[Surname].IsValid() &&
        !foundName.NameMembers[FirstName].IsValid() )
    {
        const TTextEntry::TLemmaType& surname = GetFormLemmaInfo(foundName.NameMembers[Surname]);
        resGrammems = GetGrammems(surname);
    }

    //возьмем нужные граммемы от имени, когда ничего больше нет
    if( foundName.NameMembers[FirstName].IsValid() &&
    !foundName.NameMembers[InitialName].IsValid() &&
    !foundName.NameMembers[MiddleName].IsValid() &&
    !foundName.NameMembers[InitialPatronymic].IsValid() &&
    !foundName.NameMembers[Surname].IsValid() )
    {
        const TTextEntry::TLemmaType& surname = GetFormLemmaInfo(foundName.NameMembers[FirstName]);
        resGrammems = GetGrammems(surname);
    }

    //когда граммемы взять неоткуда и неизвестен даже род,
    //присваиваем все мозможные значения
    if(foundName.NameMembers[InitialName].IsValid() &&
    !foundName.NameMembers[Surname].IsValid() )
    {
        resGrammems =  TGramBitSet(gMasculine, gFeminine, gSingular, gSurname) | NSpike::AllCases;
    }

    if( resGrammems.Test(gPlural) &&
        !resGrammems.Test(gSingular))
        return false;

    foundName.Grammems = resGrammems;
    return resGrammems.any();
}

bool TFioFinder2::CheckGrammInfo(const TFioWordSequence& nameTempalte, TVector<TFioWordSequence>& foundNames) const
{
    TVector<int> firstNameHoms;
    if(  nameTempalte.NameMembers[FirstName].IsValid() )
        GetNameHomonyms(FirstName, Entries[nameTempalte.NameMembers[FirstName].WordNum],  firstNameHoms);

    TVector<int> surnameHoms;
    if(  nameTempalte.NameMembers[Surname].IsValid() )
        GetNameHomonyms(Surname, Entries[nameTempalte.NameMembers[Surname].WordNum],  surnameHoms);

    if( firstNameHoms.size() > 0 && surnameHoms.size() > 0 )
    {
        for( size_t i = 0 ; i < firstNameHoms.size() ; i++ )
            for( size_t j = 0 ; j < surnameHoms.size() ; j++ )
            {
                TFioWordSequence newName = nameTempalte;
                newName.NameMembers[FirstName].HomNum = firstNameHoms[i];
                newName.NameMembers[Surname].HomNum = surnameHoms[j];
                if( CheckAgreements(newName) )
                {
                    newName.Fio.Surname = GetLemma(newName.NameMembers[Surname]);
                    newName.Fio.Name= GetLemma(newName.NameMembers[FirstName]);
                    foundNames.push_back(newName);
                }
            }
    }
    else
        if( firstNameHoms.size() > 0 )
        {
            for( size_t j = 0 ; j < firstNameHoms.size() ; j++ )
            {
                TFioWordSequence newName = nameTempalte;
                newName.NameMembers[FirstName].HomNum = firstNameHoms[j];
                if( CheckAgreements(newName) )
                {
                    newName.Fio.Name = GetLemma(newName.NameMembers[FirstName]);
                    foundNames.push_back(newName);
                }
            }
        }
    else
        if( surnameHoms.size() > 0 )
        {
            for( size_t i = 0 ; i < surnameHoms.size() ; i++ )
            {
                TFioWordSequence newName = nameTempalte;
                newName.NameMembers[Surname].HomNum = surnameHoms[i];
                if( CheckAgreements(newName) )
                {
                    newName.Fio.Surname = GetLemma(newName.NameMembers[Surname]);
                    foundNames.push_back(newName);
                }
            }
        }
     else //инициалы какие-нибудь
     {
        foundNames.push_back(nameTempalte);
     }

    return foundNames.size() > 0;
}

bool TFioFinder2::ApplyTemplate(TNameTemplate& nameTemplate, TVector<TFioWordSequence>& foundNames, int& curWord, int iLastWordInFIO) const
{
    if ((curWord + nameTemplate.Count) > (int)Cache.size())
        return false;
    TFioWordSequence foundName;
    for(int i = 0 ; i < nameTemplate.Count ; i++)
    {
        //если одиночная фамилия, то предсказанных не берем
        if( (nameTemplate.Count == 1) && (nameTemplate.Name[i] == Surname ) )
            return false;
        int iHom;
        if( i > 0)
            if (!CanBeLinked(Entries[curWord + i - 1], Entries[curWord + i]))
                return false;
        if(  (iHom = IsName_i(nameTemplate.Name[i], curWord + i)) == -1 )
            return false;
        foundName.NameMembers[nameTemplate.Name[i]] = TWordHomonymNum(curWord + i, iHom);
    }

    foundName.SetPair(curWord, curWord + nameTemplate.Count - 1);


    if( !nameTemplate.CheckSurnameFromDic )
    {
        if( !CheckGrammInfo(foundName, foundNames) )
            return false;
        else
            return true;
    }

    bool bFoundFromLeft = false;
    bool bFoundFromRight = false;

    int iSurnameFromRight = curWord + nameTemplate.Count;
    int iSurnameFromLeft = curWord - 1;

    TFioWordSequence foundFioFromRight = foundName;
    TFioWordSequence foundFioFromLeft = foundName;

    TVector<TFioWordSequence > namesFoundFromRight;
    TVector<TFioWordSequence > namesFoundFromLeft;

    //попытаемся слева найти
    if((iSurnameFromLeft >= 0) && (iSurnameFromLeft > iLastWordInFIO ) )
    {
        if (IsUpperForFio(Entries[iSurnameFromLeft]) &&
            CanBeLinked(Entries[iSurnameFromLeft], Entries[iSurnameFromLeft + 1]))
        {
            int iH = IsName_i(Surname, iSurnameFromLeft) ;
            if( iH != -1 )
            {
                foundFioFromLeft.NameMembers[Surname] = TWordHomonymNum(iSurnameFromLeft, iH);
                foundFioFromLeft.ChangeFirstWord(iSurnameFromLeft);
                if( CheckGrammInfo(foundFioFromLeft, namesFoundFromLeft) )
                    bFoundFromLeft = true;
                else
                    namesFoundFromLeft.clear();
            }
        }
        else
        {
            if(SurnameCanBeLinkedFromLeftThroughComma(iSurnameFromLeft) &&
               IsUpperForFio(Entries[iSurnameFromLeft]) &&
               foundFioFromLeft.NameMembers[FirstName].IsValidWordNum() &&
               foundFioFromLeft.NameMembers[MiddleName].IsValidWordNum() )

            {
                int iLastFioWord = foundFioFromLeft.NameMembers[MiddleName].WordNum;
                if( CanBeEndOfCommaSeparatedFio(iLastFioWord) )
                {
                    int iH = IsName_i(Surname, iSurnameFromLeft) ;
                    if( iH != -1 )
                    {
                        foundFioFromLeft.NameMembers[Surname] = TWordHomonymNum(iSurnameFromLeft, iH);
                        foundFioFromLeft.ChangeFirstWord(iSurnameFromLeft);
                        if( CheckGrammInfo(foundFioFromLeft, namesFoundFromLeft) )
                            bFoundFromLeft = true;
                        else
                            namesFoundFromLeft.clear();
                    }
                }
            }
        }
    }

    //попытаемся справа найти
    if (iSurnameFromRight < (int)Cache.size() &&
        IsUpperForFio(Entries[iSurnameFromRight]) &&
        CanBeLinked(Entries[iSurnameFromRight-1], Entries[iSurnameFromRight]))
    {
        int iH = IsName_i(Surname, iSurnameFromRight) ;
        if( iH != -1 )
        {
            bool surnameForNextFirstName = false;
            int iWordAfterSurname = iSurnameFromRight + 1;
            //если мы и справа нашли фамилию, то посмотрим нет ли за ней имени
            //если есть, то скорее всего это подряд идущие ФИО и фамилия спарва
            //не относится к этому ФИО
            if (bFoundFromLeft &&
                (iWordAfterSurname < (int)Cache.size()))
            {
                int iHName = IsName_i(FirstName, iWordAfterSurname);
                if ((iHName != -1) &&
                    CanBeLinked(Entries[iSurnameFromRight], Entries[iWordAfterSurname]))
                {
                    surnameForNextFirstName = true;
                }
            }

            if( !surnameForNextFirstName )
            {
                foundFioFromRight.NameMembers[Surname] = TWordHomonymNum(iSurnameFromRight, iH);
                foundFioFromRight.ChangeLastWord(iSurnameFromRight);
                //проверяем согласования и заполняем граммемы для WordSequence
                //ф-ция может размножить варианты фио для женских и мужских омонимов
                if( CheckGrammInfo(foundFioFromRight, namesFoundFromRight) )
                    bFoundFromRight = true;
                else
                    namesFoundFromRight.clear();
            }
            else
                namesFoundFromRight.clear();
        }
    }


    //if( bFoundFromRight && bFoundFromLeft &&
    //  IsBastard( AllEntries.Entries[foundFioFromRight.NameMembers[Surname].WordNum]) &&
    //  !IsBastard( AllEntries.Entries[foundFioFromLeft.NameMembers[Surname].WordNum]) )
    //{
    //  foundNames = namesFoundFromLeft;
    //  return true;
    //}

    if(bFoundFromRight && bFoundFromLeft)
    {
        int iWSCountFromLeft = GetWSCount(namesFoundFromLeft[0].FirstWord());
        int iWSCountFromRight = GetWSCount(namesFoundFromRight[0].LastWord()-1);
        if((iWSCountFromLeft <= 1) && (iWSCountFromRight > 1))
            bFoundFromRight = false;
        else
            if((iWSCountFromRight <= 1) && (iWSCountFromLeft > 1))
                bFoundFromLeft = false;
    }


    if( bFoundFromRight && bFoundFromLeft &&
        (namesFoundFromRight.ysize() > 0) && (namesFoundFromLeft.ysize() > 0) && !ForQuery)
    {
        foundNames = namesFoundFromLeft;
        foundNames.insert(foundNames.begin(), namesFoundFromRight.begin(), namesFoundFromRight.end() );
        int iFirstWord = namesFoundFromLeft[0].FirstWord();
        int iLastWord = namesFoundFromRight[0].LastWord();
        for(int i = 0 ; i < foundNames.ysize() ; i++ )
            foundNames[i].SetPair(iFirstWord, iLastWord);
        return true;
    }

    if( bFoundFromRight && (PriorityToTheRightSurname || !bFoundFromLeft) )
    {
        foundNames = namesFoundFromRight;
        return true;
    }

    if( bFoundFromLeft && (!PriorityToTheRightSurname || !bFoundFromRight) )
    {
        foundNames = namesFoundFromLeft;
        return true;
    }

    return false;
}

//ищем цепочку, описанную в TNameTemplate
bool TFioFinder2::FindFirmedName(TNameTemplate& nameTemplate, TVector<TFioWordSequence>& foundNames, int& curWord, int iLastWordInFIO) const
{
    if( !nameTemplate.CanBeFoundInQuery && ForQuery )
        return false;

    if( !ApplyTemplate(nameTemplate, foundNames, curWord, iLastWordInFIO) )
        return false;

    for( size_t i = 0 ; i < foundNames.size() ; i++ )
    {
        TFioWordSequence& foundName = foundNames[i];

        if( foundName.NameMembers[Surname].IsValid() )
        {
            foundName.Fio.Surname = GetLemma(foundName.NameMembers[Surname]);
            foundName.Fio.FoundSurname = true;
        }

        //для случая Мамай В.И.
        if( foundName.NameMembers[FirstName].IsValid() && foundName.NameMembers[InitialName].IsValid())
        {
            foundName.Fio.Surname = GetFormText(foundName.NameMembers[FirstName]);
            //foundName.m_strSurname.MakeLower();
            foundName.NameMembers[Surname] = foundName.NameMembers[FirstName];
            foundName.NameMembers[FirstName].Reset();
            int iH = IsName_i(Surname,  foundName.NameMembers[Surname].WordNum);
            if(  iH != -1 )
            {
                foundName.Fio.FoundSurname = true;
                foundName.NameMembers[Surname].HomNum = iH;
            }
        }

        if( foundName.NameMembers[FirstName].IsValid() )
            foundName.Fio.Name = GetLemma(foundName.NameMembers[FirstName]);
        if( foundName.NameMembers[MiddleName].IsValid() )
            foundName.Fio.Patronymic = GetLemma(foundName.NameMembers[MiddleName]);
        if( foundName.NameMembers[InitialName].IsValid() )
        {
            foundName.Fio.Name = GetFormText(foundName.NameMembers[InitialName]);
            foundName.Fio.InitialName = true;
        }
        if( foundName.NameMembers[InitialPatronymic].IsValid() )
        {
            foundName.Fio.Patronymic = GetFormText(foundName.NameMembers[InitialPatronymic]);
            foundName.Fio.InitialPatronymic = true;
        }
    }
    return true;
}

bool TFioFinder2::IsEndOfSentence(int iWord) const {
    if (iWord == (int)Cache.size() - 1)
        return true;
    if ((iWord < 0) || (iWord >= (int)Cache.size()))
        return false;
    if (TWordPosition::Break((i32)Entries[iWord].GetPosting()) != TWordPosition::Break((i32)Entries[iWord + 1].GetPosting()))
        return true;
    return false;
}

bool TFioFinder2::IsBegOfSentence(int iWord) const {
    if (iWord == 0)
        return true;
    if ((iWord < 0) || (iWord >= (int)Cache.size()))
        return false;
    const TTextEntry& entry = Entries[iWord];
    const TTextEntry& prevEntry = Entries[iWord - 1];
    if (TWordPosition::Break((i32)prevEntry.GetPosting()) !=  TWordPosition::Break((i32)entry.GetPosting()))
        return true;
    if (!prevEntry.GetToken() && !prevEntry.HasTokenForms() &&
        ((iWord - 1 == 0) || IsInDifferentSentences(iWord - 2, iWord - 1, &Entries[0], Entries.size())))
        return true;
    return false;
}

bool TFioFinder2::IsInNextSent(int iWord) const {
    if (iWord >= (int)Cache.size())
        return true;
    return IsBegOfSentence(iWord);
}

bool TFioFinder2::IsInPrevSent(int iWord) const {
    if (iWord < 0)
        return true;
    return IsEndOfSentence(iWord);
}

const TTextEntry::TLemmaType& TFioFinder2::GetFormLemmaInfo(const TWordHomonymNum& wh) const {
    if (!wh.IsValid())
        ythrow yexception() << "Bad TWordHomonymNum in \"GetFormLemmaInfo\"";
    return GetFormLemmaInfo(wh.WordNum, wh.HomNum);
}

const TTextEntry::TLemmaType& TFioFinder2::GetFormLemmaInfo(int iWord, int iLemma) const {
    if ((iWord < 0) || (iWord >= (int)Cache.size()))
        ythrow yexception() << "Bad iWord in  \"GetFormLemmaInfo\"";
    return Entries[iWord].GetFormLemma(iLemma);
}

TString TFioFinder2::GetFormText(const TWordHomonymNum& wh) const {
    if ((wh.WordNum < 0) || (wh.WordNum >= (int)Cache.size()))
        ythrow yexception() << "Bad wh.WordNum in \"GetFormText\".";
    return ::GetFormText(Entries[wh.WordNum]);
}

TString TFioFinder2::GetLemma(const TWordHomonymNum& wh) const {
    if ((wh.WordNum < 0) || (wh.WordNum >= (int)Cache.size()))
        ythrow yexception() << "Bad iWord in  \"GetLemma\"";

    const TTextEntry& entry = Entries[wh.WordNum];
    return ::GetLemma(entry, wh.HomNum);
}
