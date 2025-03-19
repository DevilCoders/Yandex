#pragma once

#include <kernel/fio/fio.h>
#include <kernel/indexer/direct_text/dt.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/dictlib/gleiche.h>
#include <util/generic/yexception.h>

class TTextEntry {
    struct TForm {
        const NIndexerCore::TLemmatizedToken* Value;
        TVector<const NIndexerCore::TLemmatizedToken*> Lemmas;
        explicit TForm(const NIndexerCore::TLemmatizedToken* p)
            : Value(p)
        {
        }
    };
    const NIndexerCore::TDirectTextEntry2* OrigEntry; // underlying entry
    TVector<TForm> Forms; // unique forms

    void AddUniqueForms();
    void AddFormLemmas();
public:
    typedef NIndexerCore::TLemmatizedToken TFormType; // for FormaText, Lang, Flags, Joins
    typedef NIndexerCore::TLemmatizedToken TLemmaType; // for LemmaText, Lang, StemGram, FlexGrams, GramCount, IsBastard
    typedef NIndexerCore::TDirectTextSpace TSpaceType;

    explicit TTextEntry(const NIndexerCore::TDirectTextEntry2& e);
    ui32 GetFormCount() const {
        return (ui32)Forms.size();
    }
    const TFormType& GetForm(size_t i) const {
        if (Forms.empty())
            ythrow yexception() << "no forms";
        return *Forms[i].Value;
    }
    size_t GetFormLemmaCount() const {
        return (Forms.size() ? Forms.back().Lemmas.size() : 0);
    }
    const TLemmaType& GetFormLemma(size_t i) const {
        if (Forms.empty())
            ythrow yexception() << "no forms";
        const TForm& f = Forms.back();
        if (i >= f.Lemmas.size())
            ythrow yexception() << "index out of range";
        return *f.Lemmas[i];
    }
    const TSpaceType& GetSpace(size_t i) const {
        return OrigEntry->Spaces[i];
    }
    size_t GetSpaceCount() const {
        return OrigEntry->SpaceCount;
    }
    TPosting GetPosting() const {
        return OrigEntry->Posting;
    }
    const wchar16* GetToken() const {
        return OrigEntry->Token.data();
    }
    bool HasTokenForms() const {
        return OrigEntry->LemmatizedToken != nullptr;
    }
};

//typedef std::bitset<gMax> TGramBitSet;
//
//extern TGramBitSet AllGenders;
//extern TGramBitSet AllCases;
//extern TGramBitSet AllNumbers;
//extern TGramBitSet GR_DEF;
//extern TGramBitSet GR_NULL;
//
//
//
//#define _QM(gr) (GR_DEF << gr)

bool IsUpper(const TTextEntry& e);
bool HasTwoUpper(const TTextEntry& entry);
bool IsBastard(const TTextEntry& entry);
bool IsInDifferentSentences(int iW1, int iW2, const TTextEntry* entries, int count);
bool HasGrammem(const TTextEntry& entry, EGrammar gr);
int HasGrammem_i(const TTextEntry& entry, EGrammar gr);
bool HasGrammem(const TTextEntry::TLemmaType& lemmInfo, EGrammar gr);
bool HasOnlyGrammem(const TTextEntry::TLemmaType& lemmInfo, EGrammar gr);
bool HasPOSes(const TTextEntry::TLemmaType& lemmInfo, const TGramBitSet& poses);
bool HasOnlyPOSes(const TTextEntry& entry, const TGramBitSet& poses);
bool HasOnlyGrammems(const TTextEntry& entry, const TGramBitSet& grammems);
bool HasOneOfGrammems(const TTextEntry::TLemmaType& lemmInfo, const TGramBitSet& grammems);
bool HasOnlyGrammem(const TTextEntry& entry, const EGrammar& grm);
bool HasCommonLemmas(const TTextEntry& entry1, const TTextEntry& entry2);


bool ContainsGrammem(const char* grammems, EGrammar gr);
bool ContainsOtherGrammem(const char* grammems, EGrammar gr);
bool ContainsOnlyGrammem(const char* grammems, EGrammar gr);

TString GetFormText(const TTextEntry& entry);
TString GetLemma(const TTextEntry& entry, size_t iLemma);

bool IsName(ENameType type, bool canBeBastard, const char* stemGram, bool isBastard);

inline int GetFormLemmaInfoCount(const TTextEntry& entry) {
    return (int)entry.GetFormLemmaCount();
}

TGramBitSet GetGrammems(const TTextEntry::TLemmaType& lemmInfo);
TGramBitSet GetGrammems(const TTextEntry::TLemmaType& lemmInfo, TVector<TGramBitSet>& flexGram);
//TGramBitSet ToGrammemsSet(const char* grammems);
//TString GrammemsSet2Str(const TGramBitSet& gr);


TGramBitSet Gleiche(const TTextEntry::TLemmaType& w1, const TTextEntry::TLemmaType& w2, ::NGleiche::TGleicheFunc gleicheFunc);
TGramBitSet GleicheGenderNumberCase(const TTextEntry::TLemmaType& w1, const TTextEntry::TLemmaType& w2);
TGramBitSet GleicheCaseNumber(const TTextEntry::TLemmaType& w1, const TTextEntry::TLemmaType& w2);
