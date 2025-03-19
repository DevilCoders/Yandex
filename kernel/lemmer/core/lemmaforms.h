#pragma once

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <library/cpp/token/formtype.h>
#include <library/cpp/token/token_structure.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/generic/ylimits.h>
#include <util/charset/wide.h>

#include "langcontext.h"
#include "lemmer.h"
#include "language.h"

class TFormDecimator;

class TLemmaForms {
    friend class TWordInstance;
    friend class TWordInstanceUpdate;
    friend class TLemmaFormsDeserializer;

public:
    typedef i32 TWeight;
    static constexpr TWeight NoWeight = 0;

    // enum, который сделает все вызовы конструкторов TFormWeight читаемыми
    enum EMatchType : bool {
        InexactMatch = false,
        ExactMatch = true,
    };

    struct TFormWeight {
        TWeight Weight;              // вес формы для поиска (пока так и не пригодился)
        NSpike::TGrammarBunch Grams; // грамматики формы
        bool IsExact;                // является ли в точности запрошенной формой
    public:
        TFormWeight(TWeight weight, EMatchType isExact = InexactMatch)
            : Weight(weight)
            , IsExact(isExact)
        {
        }

        TFormWeight(EMatchType isExact = InexactMatch)
            : TFormWeight(NoWeight, isExact)
        {
        }

        // TFormWeight (true) присвоит 1 в вес, а не true в IsExact, то есть сотворит зло. Не надо так.
        TFormWeight(bool) = delete;

        bool operator==(const TFormWeight& ot) const {
            return Weight == ot.Weight;
        }
        TFormWeight& operator|=(const TFormWeight& w) {
            if (Weight > w.Weight)
                Weight = w.Weight;
            Grams.insert(w.Grams.begin(), w.Grams.end());
            IsExact = IsExact || w.IsExact;
            return *this;
        }
    };
    typedef THashMap<TUtf16String, TFormWeight> TFormMap;

protected:
    enum TSetFormtypeRes {
        sftOk,
        sftFailed,
        sftMustBeDeleted
    };

protected:
    TAtomicSharedPtr<TYandexLemma> YandexLemma; // яндекс-лемма, из которой была сгенерированна данная (если есть)
    TUtf16String Form;                          // изначальная форма
    TUtf16String NormalizedForm;                // нормализованная изначальная форма (диакритика + ренуха)
    TFormType FormType;                         // обычная форма / точная форма (!) / точная лемма (!!)
    TUtf16String Lemma;                         // текст леммы
    ELanguage Language;                         // язык

    NLemmer::TLllLevel ExactLemma; // форма похожа на лемму (в русском языке достаточно быть именительным падежом или инфинитивом)
    bool Best;                     // изначально == (bool)ExactLemma, можно сбрасывать и ставить. Вообще говоря, вредный параметр
    ui32 Quality;                  // словарность / бастардность (см. TYandexLemma)
    bool StopWord;                 // стоп-словность
    EStickySide Stickiness;        // липучесть стоп-слова
    bool Overridden;               // лемму достали из фикс-листа

    TGramBitSet StemGrammar;           // грамматика всей парадигмы
    TVector<TGramBitSet> FlexGrammars; // грамматика конкретной формы

    TCowPtr<TAtomicSharedPtr<TFormMap>> Forms; // формы (может не быть)
    const TFormDecimator* Decimator;           // используемый дециматор форм

private:
    void AddExactForms(const TLanguage* lang);
    void GenerateForms();
    void RestrictFormsByLemma();
    void RestrictFormsNoLemma();

    inline void AddFormaMergeInt(const TUtf16String& form, const TFormWeight& weight, const NSpike::TGrammarBunch& grammems);
    inline TFormWeight& AddFormaMergeInt(const TUtf16String& form, const TFormWeight& weight) {
        return (*Forms.Mutable())[form] |= weight;
    }

private: // used by TWordInstance
    TLemmaForms(const TFormMap::value_type& form, const TLemmaForms& lemma);
    TSetFormtypeRes SetFormType(TFormType newType);

public:
    TLemmaForms(const TLemmaForms&) = default;
    TLemmaForms& operator=(const TLemmaForms&) = default;
    TLemmaForms(TLemmaForms&& other) = default;

    TLemmaForms(const TYandexLemma& lemma, TFormType fType = fGeneral, bool generateForms = true, const TFormDecimator* decimator = nullptr);
    TLemmaForms(const TUtf16String& form = TUtf16String(), const TUtf16String& lemma = TUtf16String(), ELanguage lang = LANG_UNK,
                bool isExact = false, ui32 quality = TYandexLemma::QFoundling, TFormType fType = fGeneral,
                const TGramBitSet& stemGrammar = TGramBitSet());

    TFormWeight& AddFormaMerge(const TUtf16String& form, const TFormWeight& weight = TFormWeight()) {
        if (!Forms)
            Forms.Reset(new TFormMap);
        return AddFormaMergeInt(form, weight);
    }
    TFormWeight& AddFormaMerge(const TUtf16String& form, const TUtf16String& suffix, const TFormWeight& weight = TFormWeight()) {
        return AddFormaMerge(!suffix ? form : (form + suffix), weight);
    }

    TFormType GetFormType() const {
        return FormType;
    }

    bool IsBadRequest() const {
        return YandexLemma && YandexLemma->IsBadRequest();
    }
    bool IsIntrusiveBastard(bool useFixList) const;

    const TUtf16String& GetNormalizedForm() const {
        return NormalizedForm;
    }
    const TUtf16String& GetInitialForm() const {
        return Form;
    }

    const TUtf16String& GetLemma() const {
        return Lemma;
    }

    int GetLanguageVersion() const {
        return YandexLemma ? YandexLemma->GetLanguageVersion() : -1;
    }

    ELanguage GetLanguage() const {
        return Language;
    }

    const TGramBitSet& GetStemGrammar() const {
        return StemGrammar;
    }

    const TVector<TGramBitSet>& GetFlexGrammars() const {
        return FlexGrammars;
    }

    void SetBest(bool best = true) {
        Best = best;
    }

    NLemmer::TLllLevel IsExactLemma() const {
        return ExactLemma;
    }

    bool IsBest() const {
        return Best;
    }

    bool IsOverridden() const {
        return Overridden;
    }

    bool IsBastard(ui32 qual = TYandexLemma::QAnyBastard) const;

    ui32 GetQuality() const {
        return Quality;
    }

    void SetStopWord(bool stopWord, EStickySide stickiness);
    void SetStopWord(bool stopWord = true);

    bool IsStopWord() const {
        return StopWord;
    }

    EStickySide GetStickiness() const {
        return Stickiness;
    }

    const TFormDecimator* GetDecimator() const {
        return Decimator;
    }

    bool HasExactForm() const;

    const TFormMap& GetForms() const {
        Y_ASSERT(FormsGenerated());
        return *Forms;
    }

    bool FormsGenerated() const {
        return static_cast<bool>(Forms);
    }

    // Returned ref should not be used after copying parent TLemmaForms
    // due to Forms copy-on-write behaviour
    TFormMap& ObtainForms() {
        if (!FormsGenerated())
            GenerateForms();
        return *Forms.Mutable();
    }

    void ClearForms() {
        Forms.Reset(new TFormMap);
    }

    void DiscardForms() {
        Forms.Reset(nullptr);
    }

    void SetDistortion() {
        StemGrammar.Set(gDistort);
    }

    void Swap(TLemmaForms& other);

    size_t NumForms() const {
        Y_ASSERT(FormsGenerated());
        return Forms->size();
    }

    bool HasForm(const TUtf16String& form) const {
        Y_ASSERT(FormsGenerated());
        return Forms->find(form) != Forms->end();
    }

    TWeight GetFormDistance(const TUtf16String& form) const {
        Y_ASSERT(FormsGenerated());
        TFormMap::const_iterator it = Forms->find(form);
        return (it == Forms->end()) ? NoWeight : it->second.Weight;
    }

    bool HasWholeStemGram(const TGramBitSet& gr) const;
    bool HasWholeFlexGram(const TGramBitSet& gr) const;
    bool HasWholeGram(const TGramBitSet& gr) const;

    bool HasAnyStemGram(const TGramBitSet& gr) const;
    bool HasAnyFlexGram(const TGramBitSet& gr) const;
    bool HasAnyGram(const TGramBitSet& gr) const;

    bool HasStemGram(EGrammar gr) const;
    bool HasFlexGram(EGrammar gr) const;
    bool HasGram(EGrammar gr) const;
    bool HasNormalForm() const {
        return !YandexLemma || YandexLemma->GetMinDistortion() == 0;
    }

    inline const TYandexLemma* GetOriginalLemma() const {
        return YandexLemma.Get();
    }

    static TLemmaForms MakeExactDefaultLemmaForms(const TUtf16String& text);
};

bool operator<(const TLemmaForms& a, const TLemmaForms& b);

namespace NLemmerAux {
    TUtf16String LowCase(const TUtf16String& form, ELanguage lang);
    TUtf16String NormalizeForm(const TUtf16String& form, ELanguage lang, bool derenyx = true);
}
