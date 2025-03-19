#include "gramhelper2.h"

#include <kernel/fio/fio.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>

//TGramBitSet AllGenders( TGramBitSet((const int)7) << (const int)gFeminine );
//TGramBitSet AllCases( TGramBitSet((const int)63) << (const int)gNominative );
//TGramBitSet AllNumbers( TGramBitSet((const int)3) << (const int)gSingular );
//TGramBitSet GR_DEF(1);
//TGramBitSet GR_NULL(0);

using namespace NIndexerCore;
using NTGrammarProcessing::ch2tg;

TTextEntry::TTextEntry(const NIndexerCore::TDirectTextEntry2& e)
    : OrigEntry(&e)
{
    AddUniqueForms();
    AddFormLemmas();
}

void TTextEntry::AddUniqueForms() {
    for (ui32 i = 0; i < OrigEntry->LemmatizedTokenCount; ++i) {
        const TLemmatizedToken* token = &OrigEntry->LemmatizedToken[i];
        size_t j = 0;
        for (; j < Forms.size(); ++j) {
            if (FormsEqual(Forms[j].Value, token))
                break;
        }
        if (j == Forms.size()) // not found -> add
            Forms.push_back(TForm(token));
    }
}

void TTextEntry::AddFormLemmas() {
    for (size_t i = 0; i < Forms.size(); ++i) {
        TForm& form = Forms[i];
        for (size_t j = 0; j < OrigEntry->LemmatizedTokenCount; ++j) {
            const TLemmatizedToken* token = &OrigEntry->LemmatizedToken[j];
            if (FormsEqual(form.Value, token))
                form.Lemmas.push_back(token);
        }
    }
}

//кроме первой буквы - есть еще одна заглавная
bool HasTwoUpper(const TTextEntry& entry) {
    if (!entry.GetFormCount())
        return false;
    const TTextEntry::TFormType& token = entry.GetForm(0);

    if (!token.FormaText)
        return false;

    if (!csYandex.IsUpper((unsigned char)token.FormaText[0]))
        return false;
    const size_t n = strlen(token.FormaText);
    for( size_t i = 1 ; i < n ; i++ )
        if( csYandex.IsUpper((unsigned char)token.FormaText[i]) )
            return true;

    return false;
}

bool IsUpper(const TTextEntry& entry) {
    if (!entry.GetFormCount())
        return false;
    const TTextEntry::TFormType& form = entry.GetForm(0);
    return (form.FormaText != nullptr) && (form.Flags & FORM_TITLECASE) ||
        (entry.GetToken() && ::IsUpper(*entry.GetToken()));
}

bool IsBastard(const TTextEntry& entry) {
    if (!entry.GetFormCount())
        return false;

    //если хотя бы один не предсказан, значит есть в словаре
    for (size_t i = 0 ; i < entry.GetFormLemmaCount(); ++i) {
        if (!entry.GetFormLemma(i).IsBastard)
            return false;
    }
    return true;
}

bool IsInDifferentSentences(int iW1, int iW2, const TTextEntry* entries, int count) {
    if ((iW1 < 0) || (iW2 < 0) || (iW1 >= count) || (iW2 >= count))
        return false;
    return TWordPosition::Break((i32)entries[iW1].GetPosting()) !=  TWordPosition::Break((i32)entries[iW2].GetPosting());
}

bool HasCommonLemmas(const TTextEntry& entry1, const TTextEntry& entry2) {
    for (size_t i = 0; i < entry1.GetFormLemmaCount(); ++i) {
        for (size_t j = 0; j < entry2.GetFormLemmaCount(); ++j) {
            if( !strcmp(GetLemma(entry1, i).data(), GetLemma(entry2, j).data()) )
                return true;
        }
    }
    return false;
}

bool HasGrammem(const TTextEntry& entry, EGrammar gr) {
    return HasGrammem_i(entry, gr) != -1;
}

int HasGrammem_i(const TTextEntry& entry, EGrammar gr) {
    int n = entry.GetFormLemmaCount();
    for (int i = 0 ; i < n; ++i)
        if (HasGrammem(entry.GetFormLemma(i), gr))
            return i;
    return -1;
}

bool HasOneOfGrammems(const TTextEntry::TLemmaType& lemmInfo, const TGramBitSet& grammems)
{
    if( !lemmInfo.StemGram )
        return false;
    for( size_t j = 0; lemmInfo.StemGram[j]; j++ )
    {
        if (grammems.Test(ch2tg(lemmInfo.StemGram[j])))
            return true;
    }

    for( size_t i = 0 ; i < lemmInfo.GramCount ; i++ )
    {
        for( size_t j = 0; lemmInfo.FlexGrams[i][j] ; j++ )
        {
            if(grammems.Test(ch2tg(lemmInfo.FlexGrams[i][j])))
                return true;
        }
    }
    return false;
}

bool HasGrammem(const TTextEntry::TLemmaType& lemmInfo, EGrammar gr)
{
    if( !lemmInfo.StemGram )
        return false;
    if( ContainsGrammem(lemmInfo.StemGram, gr) )
        return true;
    for( size_t i = 0 ; i < lemmInfo.GramCount ; i++ )
        if( ContainsGrammem(lemmInfo.FlexGrams[i], gr) )
            return true;
    return false;
}

bool HasOnlyGrammem(const TTextEntry::TLemmaType& lemmInfo, EGrammar gr)
{
    if( !lemmInfo.StemGram )
        return false;
    if( ContainsOnlyGrammem(lemmInfo.StemGram, gr) )
        return true;

    bool bHasGrammem = false;
    bool bHasOtherGrammemFromThisClass = false;

    for( size_t i = 0 ; i < lemmInfo.GramCount ; i++ )
    {
        bHasGrammem = ContainsGrammem(lemmInfo.FlexGrams[i], gr) || bHasGrammem;
        bHasOtherGrammemFromThisClass = ContainsOtherGrammem(lemmInfo.FlexGrams[i], gr) || bHasOtherGrammemFromThisClass;
    }
    return bHasGrammem && !bHasOtherGrammemFromThisClass;
}


bool HasPOSes(const TTextEntry::TLemmaType& lemmInfo, const TGramBitSet& poses)
{
    if( lemmInfo.StemGram == nullptr )
        return false;
    if( strlen(lemmInfo.StemGram) == 0 )
        return false;
    //TString s1 = GrammemsSet2Str(poses);
    //TString s2 = GrammemsSet2Str(NSpike::MakeMask((unsigned char)(lemmInfo.StemGram[0])));
    return poses.Test(ch2tg(lemmInfo.StemGram[0]));
}

bool HasOnlyGrammem(const TTextEntry& entry, const EGrammar& grm) {
    int n = entry.GetFormLemmaCount();
    for (int i = 0; i < n; i++) {
        if (!HasGrammem(entry.GetFormLemma(i), grm))
            return false;
    }
    return true;
}

bool HasOnlyGrammems(const TTextEntry& entry, const TGramBitSet& grammems) {
    for (size_t i = 0; i < entry.GetFormLemmaCount(); ++i) {
        if (!HasOneOfGrammems(entry.GetFormLemma(i), grammems))
            return false;
    }
    return true;
}

bool HasOnlyPOSes(const TTextEntry& entry, const TGramBitSet& poses) {
    int n = entry.GetFormLemmaCount();
    for (int i = 0 ; i < n; ++i) {
        if (!HasPOSes(entry.GetFormLemma(i), poses))
            return false;
    }
    return true;
}

TString GetFormText(const TTextEntry& entry) {
    TString res;
    ui32 n = entry.GetFormCount();
    for (ui32 i = 0; i < n; ++i) {
        res += entry.GetForm(i).FormaText;
        if (i < n - 1)
            res += '-';
    }
    return res;
}

TString GetLemma(const TTextEntry& entry, size_t iLemma) {
    TString res;
    const ui32 n = entry.GetFormCount();
    for (ui32 i = 0; i < n - 1; i++ ) {
        const char* strForm = entry.GetForm(i).FormaText;
        if( strcspn(strForm, ".-'") < strlen(strForm) )
            continue;
        res += strForm;
        if (i < n - 1)
            res += '-';
    }
    res += entry.GetFormLemma(iLemma).LemmaText;
    return res;
}

bool IsName(ENameType type, bool canBeBastard, const char* stemGram, bool isBastard)
{
    if( !canBeBastard && isBastard )
        return  false;

    //не верим предсказанному имени
    if( (type == FirstName) && isBastard )
        return false;

    EGrammar grammem = gMax;
    switch(type)
    {
        case FirstName: grammem = gFirstName; break;
        case Surname: grammem = gSurname; break;
        case MiddleName: grammem = gPatr; break;
        default: break;
    }

    if( ContainsGrammem(stemGram, grammem ) &&  ContainsGrammem(stemGram, gSubstantive))
        return true;

    return false;
}


bool ContainsOtherGrammem(const char* grammems, EGrammar gr)
{
    if( !grammems )
        return false;

    const TGramClass cl = GetClass((char)gr);
    if (!cl)
        return false;
    int i = 0;
    while(  grammems[i] )
    {
        if( ( GetClass((char)grammems[i]) == cl ) &&
            (unsigned char)grammems[i] != gr )
                return true;
        i++;
    }
    return false;
}


bool ContainsGrammem(const char* grammems, EGrammar gr)
{
    if( !grammems )
        return false;
    return strchr(grammems, (char)gr) != nullptr;
}


bool ContainsOnlyGrammem(const char* grammems, EGrammar gr)
{
    if( !grammems )
        return false;

    const TGramClass cl = GetClass((char)gr);
    int i = 0;
    bool bRes = false;
    while(  grammems[i] )
    {
        if( GetClass((char)grammems[i]) == cl )
        {
            if(cl && (unsigned char)grammems[i] != gr )
                    return false;
            if( (unsigned char)grammems[i] == gr )
                bRes = true;
        }
        i++;
    }
    return bRes;
}


TGramBitSet GleicheGenderNumberCase(const TTextEntry::TLemmaType& w1, const TTextEntry::TLemmaType& w2) {
    return Gleiche(w1, w2, NGleiche::GenderNumberCaseCheck);
}

//TGramBitSet ToGrammemsSet(const char* grammems)
//{
//    if(!grammems)
//        return 0;
//    TGramBitSet res = 0;
//    int i = 0;
//    while( grammems[i] )
//        res |= NSpike::MakeMask((unsigned char)grammems[i++]);
//    return res;
//}

TGramBitSet GetGrammems(const TTextEntry::TLemmaType& lemmInfo) {
    if( !lemmInfo.StemGram )
        return TGramBitSet();

    TGramBitSet res;

    res |=  TGramBitSet::FromBytes(lemmInfo.StemGram);

    size_t i = 0;
    for( i = 0 ; i < lemmInfo.GramCount ; i++ )
    {
        res |=  TGramBitSet::FromBytes(lemmInfo.FlexGrams[i]);
    }

    return res;
}

//TString GrammemsSet2Str(const TGramBitSet& gr)
//{
//    TString res;
//    unsigned char c = gBefore + 1;
//    for( ; c < gMax ; c++ )
//    {
//        if( (gr & NSpike::MakeMask(c)) != 0 )
//        {
//            const char *g = TGrammarIndex::GetName(NTGrammarProcessing::ch2tg(c));
//            if( g )
//            {
//                res += g;
//                res += ' ';
//            }
//        }
//    }
//    return res;
//}

TGramBitSet GetGrammems(const TTextEntry::TLemmaType& lemmInfo, TVector<TGramBitSet>& flexGram) {
    if( !lemmInfo.StemGram )
        return TGramBitSet();

    TGramBitSet res;

    res = TGramBitSet::FromBytes(lemmInfo.StemGram);

    size_t i = 0;
    for( i = 0 ; i < lemmInfo.GramCount ; i++ )
    {
        TGramBitSet gr = TGramBitSet::FromBytes(lemmInfo.FlexGrams[i]);
        flexGram.push_back(gr);
    }

    return res;
}

TGramBitSet Gleiche(const TTextEntry::TLemmaType& w1, const TTextEntry::TLemmaType& w2, NGleiche::TGleicheFunc gleicheFunc) {
    return NGleiche::GleicheGrammems(w1.StemGram, w2.StemGram, w1.FlexGrams, w1.GramCount, w2.FlexGrams,w2.GramCount, gleicheFunc);
}

TGramBitSet GleicheCaseNumber(const TTextEntry::TLemmaType& w1, const TTextEntry::TLemmaType& w2) {
    return Gleiche(w1, w2, NGleiche::CaseNumberCheck);
}
