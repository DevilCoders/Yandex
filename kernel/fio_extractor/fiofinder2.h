#pragma once

#include <kernel/lemmer/core/language.h>
#include <kernel/indexer/direct_text/dt.h>


#include "fiowordsequence.h"
#include "gramhelper2.h"

const char FIO_NAME_POSTFIX[10] = "fi";
const char FIO_PATRONYMIC_POSTFIX[10] = "fo";

class TFioFinder2 {

    //чтобы 1000 раз не авызывать ф-цию IsName
    //запоминаем ее результат для каждого TDirectTextEntry
    struct SWordFioInfo
    {
        SWordFioInfo()
        {
            Checked= false;
            FioFlags = 0;
        }

        void Reset()
        {
            Checked= false;
            FioFlags = 0;
            HomonymFioFlags.clear();
        }

        bool Checked;
        long FioFlags;
        TVector<long> HomonymFioFlags;
    };

    TVector<TTextEntry> Entries;

public:

    struct TFilterCallback {
        enum EResult {
            GOOD_FIO = 0,
            BAD_FIO
        };
        virtual ~TFilterCallback() { }
        virtual EResult operator()(const TFioFinder2& finder,
                                   const TFioWordSequence& candidate) const = 0;
    };

    struct TNameTemplate
    {
        ENameType Name[3];
        long      Count;
        EFIOType  FIOType;
        bool      CheckSurnameFromDic;//должны ли мы пытаться искать словарную фамилию слева или справа
        bool      CanBeFoundInQuery;//распознавать ли этот шаблон при анализе запроса
    };

    static const long FirmedNameTemplatesCount;

    TFioFinder2(const NIndexerCore::TDirectTextEntry2* entries, size_t count, int forQurey = 0);
    void FindFios(TVector<TFioWordSequence>& fioWSes, TVector<int>& potentialSurnames, size_t maxFioCount = Max()) const;

    const TTextEntry* GetEntries() const {
        return &Entries[0];
    }
    size_t GetEntryCount() const {
        return Entries.size();
    }
    const TTextEntry& GetEntry(size_t i) const {
        return Entries[i];
    }

    bool IsName(size_t iEntry) const;
    bool IsName(ENameType type, size_t iEntry, bool canBeBastard = true) const;
    bool IsName(ENameType type, size_t iEntry, size_t iHom, bool canBeBastard = true) const;
    bool IsName(ENameType type, const TTextEntry::TLemmaType& lemmInfo, bool canBeBastard = true) const;

    bool HasSuspiciousPatronymic(TFioWordSequence& fioWS) const;
    void AddToSurnames(std::set<TString>& surnameBeginnings, bool bSuspiciousPatronomyc, const TFioWordSequence& fioWS) const;

private:
    bool FindFirmedName(TNameTemplate& nameTemplate, TVector<TFioWordSequence>& foundNames, int& curWord, int iLastWordInFIO) const;
    bool ApplyTemplate(TNameTemplate& nameTemplate, TVector<TFioWordSequence>& foundNames, int& curWord, int iLastWordInFIO) const;
    bool CheckGrammInfo(const TFioWordSequence& nameTempalte, TVector<TFioWordSequence>& foundNames) const;
    bool CanBeLinked(const TTextEntry& e1, const TTextEntry& e2) const;
    bool HasCloseQuote(const TTextEntry& e) const;
    bool NextHasOpenQuote(const TTextEntry& e) const;
    int IsName_i(ENameType type, size_t iEntry, bool canBeBastard = true) const;
    bool CanBeEndOfCommaSeparatedFio(int iW) const;
    bool SurnameCanBeLinkedFromLeftThroughComma(int iSurname) const;

    bool GetNameHomonyms(ENameType type, const TTextEntry& entry, TVector<int>& homs) const;
    bool IsInitial(const TTextEntry& entry) const;
    bool CanBeName(const TTextEntry& entry) const;
    bool EnlargeIOname(TFioWordSequence& foundName, int lastWordInFIO) const;
    bool EnlargeIname(TFioWordSequence& foundName, int lastWordInFIO) const;
    bool EnlargeIOnameIn(TFioWordSequence& foundName, int lastWordInFIO) const;
    bool EnlargeIOnameInIn(TFioWordSequence& foundName, int lastWordInFIO) const;
    bool EnlargeInameIn(TFioWordSequence& foundName, int lastWordInFIO) const;
    bool EnlargeBySurname(TFioWordSequence& foundName, int lastWordInFIO, int nextWord, int prevWord) const;
    void AddSingleSurname(int iW, TVector< TFioWordSequence >& foundNames) const;
    bool AddFoundFio(TVector< TFioWordSequence >& foundNames, TFioWordSequence& foundName) const;
    void ChooseVariants(TVector< TFioWordSequence >& addedFioVariants, TVector< TFioWordSequence >& foundNames) const;
    TGramBitSet FirstNameAgreement(const TWordHomonymNum& word_ind, const TWordHomonymNum& first_name_ind) const;
    bool CheckAgreements(TFioWordSequence& foundName) const;
    bool GleichePredictedSurname(TFioWordSequence& foundName, int iW, int& newH, TGramBitSet& commonGrammems) const;
    bool HasPunct(const TTextEntry& e) const;
    bool CanBeSurnameFromTheLeft(int iW, bool bAtTheEndOfSent, TFioWordSequence& foundName) const;
    bool CanBeSurnameFromTheRight(int iW, int iWName) const;
    bool NameCanBeSurnameFromTheRight( TFioWordSequence& foundName, int iW, int& iSurnameHFromRight) const;
    void FillWordFioInfo(size_t iEntry) const;
    bool CanBeFirstName(const TTextEntry& entry , const TTextEntry::TLemmaType& lemmInfo) const;
    void ChangePatronymicToSurnameForQuery(TFioWordSequence& foundName) const;
    bool IsUpperForFio(const TTextEntry& entry) const;
    int GetWSCount(int iW) const;

    bool IsEndOfSentence(int iWord) const;
    bool IsBegOfSentence(int iWord) const;
    bool IsInNextSent(int iWord) const;
    bool IsInPrevSent(int iWord) const;
public:
    const TTextEntry::TLemmaType& GetFormLemmaInfo(const TWordHomonymNum& wh) const;
    const TTextEntry::TLemmaType& GetFormLemmaInfo(int iWord, int iLemma) const;
    TString GetFormText(const TWordHomonymNum& wh) const;
    TString GetLemma(const TWordHomonymNum& wh) const;

    void SetFilter(TFilterCallback* callback = nullptr) {
        Filter.Reset(callback);
    }

private:
    bool PriorityToTheRightSurname;
    size_t FirstWordInCurSent;
    int ForQuery; //0 - не для запроса, 1- для запросаб 2 - для запроса и самое начало запроса
    THolder<TFilterCallback> Filter;
    mutable TVector<SWordFioInfo> Cache;

};
