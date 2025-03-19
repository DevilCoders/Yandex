#pragma once

#include <kernel/search_types/search_types.h>
#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <library/cpp/token/formtype.h>
#include <library/cpp/token/token_structure.h>
#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>
#include <util/generic/ylimits.h>

#include "langcontext.h"
#include "language.h"
#include "lemmaforms.h"
#include "lemmer.h"
#include "swapvector.h"

class TWordInstance;
class TWordInstanceUpdate;
class TFormDecimator;

class TWordInstance {
    friend class TWordInstanceUpdate;

public:
    class TLemmasVector: public TSwapVector<TLemmaForms> {
        friend class TWordInstanceUpdate;
        friend class TWordInstance;
        friend class TWordNode;

    public:
        bool AreFormsGenerated() const;
        void DiscardForms();
    };

    typedef TLemmasVector::const_iterator TCLemIterator;

protected:
    TUtf16String Form;             // форма, как её спросил пользователь
    TUtf16String NormalizedForm;   // она же с приведённой диакритикой
    size_t TokenCount;             // кол-во токенов
    TLemmasVector Lemmas;          // леммы
    TLemmasVector RedundantLemmas; // "срубленные" леммы (например, из лишних языков)
    TLangMask LangMask;            // языки всех лемм
    TCharCategory CaseFlags;       // флаги регистра пользовательской формы
protected:
    void SetNormalizedForm(const TUtf16String& form);
    void ReduntLemma(size_t j);
    void AddLemma(const TYandexLemma& lemma, const TLanguageContext& langInfo, TFormType formType, bool generateForms);
    void InitLemmas(const TUtf16String& tok, const TCharSpan& span, const TLanguageContext& lang, TFormType formType, bool generateForms, bool useFixList);
    void SetNonLemmerFormType(TFormType formType);

    void Clear();

public:
    TWordInstance();
    virtual ~TWordInstance() {
    }

    void Init(const TUtf16String& word, const TCharSpan& span, const TLanguageContext& langInfo = TLanguageContext(), TFormType formType = fGeneral, bool generateForms = true, bool useFixList = true);
    void Init(const TVector<const TYandexLemma*>& lemmas, const TLanguageContext& langInfo = TLanguageContext(), TFormType formType = fGeneral, bool generateForms = true);
    void GenerateAllForms(const TFormDecimator* decimator = nullptr);

    void AddPlainLemma(const TUtf16String& lemma, ELanguage lang, bool stopWord = false);
    TLemmaForms& AddLemma(const TLemmaForms& lm);

    bool IsBastard(ui32 qual = TYandexLemma::QAnyBastard, const TLangMask& lang = ~TLangMask()) const;

    TLemmasVector& GetLemmas() {
        return Lemmas;
    }

    bool AreFormsGenerated() const {
        return Lemmas.AreFormsGenerated() && RedundantLemmas.AreFormsGenerated();
    }

    void DiscardForms() {
        Lemmas.DiscardForms();
        RedundantLemmas.DiscardForms();
    }

    void CleanBestFlag();

    const TLemmasVector& GetLemmas() const {
        return Lemmas;
    }
    const TLemmasVector& GetRedundantLemmas() const {
        return RedundantLemmas;
    }

    const TUtf16String& GetNormalizedForm() const {
        return NormalizedForm;
    }

    const TUtf16String& GetInitialForm() const {
        return Form;
    }

    const TLangMask& GetLangMask() const {
        return LangMask;
    }

    size_t GetTokenCount() const {
        return TokenCount;
    }

    TCharCategory GetCaseFlags() const {
        return CaseFlags;
    }
    TCharCategory AddCase(TCharCategory cs) {
        return CaseFlags |= cs;
    }
    TCharCategory SubCase(TCharCategory cs) {
        return CaseFlags &= ~cs;
    }

    void SetStopWord(bool stopWord, EStickySide stickiness);
    void SetStopWord(bool stopWord = true);

    bool IsStopWord() const;
    bool IsStopWord(EStickySide& stickiness) const;

    bool SetFormType(TFormType newType);
    TFormType GetFormType() const;
    TLemmaForms::TSetFormtypeRes SetFormTypeOrErase(size_t lemmNum, TFormType newType);

    TCLemIterator LemsEnd() const {
        return Lemmas.end();
    }

    TCLemIterator LemsBegin() const {
        return Lemmas.begin();
    }

    size_t NumLemmas() const {
        return Lemmas.size();
    }

    // to move out
    bool HasLemma(const TUtf16String& lemma) const {
        for (const auto& Lemma : Lemmas) {
            if (Lemma.GetLemma() == lemma)
                return true;
        }
        return false;
    }

    bool HasFormGenerated(const TUtf16String& form) const {
        for (const auto& Lemma : Lemmas) {
            if (Lemma.FormsGenerated() && Lemma.HasForm(form))
                return true;
        }
        return false;
    }

    bool HasGram(EGrammar gr) const {
        for (const auto& lemma : Lemmas) {
            if (lemma.HasGram(gr))
                return true;
        }
        return false;
    }
};

class TWordInstanceUpdate {
    TWordInstance& Wi;

public:
    TWordInstanceUpdate(TWordInstance& wi)
        : Wi(wi)
    {
    }

    TLangMask SelectBest(bool removeBad = true, TLangMask preferableLanguage = TLangMask());
    TLangMask FilterLemmas(TLangMask lang);
    void FlattenLemmas(TLangMask lang, bool bastardsOnly = false);
    void ShrinkLemmas(TLangMask lang, bool bastardsOnly = false);
    void SpreadFixListLemmas();
    void AddDefaultLemma();
    void UpdateRedundantLemmas();
    void InjectRedundantLemmas(); // Appends redundant lemmas to main lemmas
    void RepairLLL(bool removeOdd = false);
    void ShrinkIntrusiveBastards(bool useFixList = true);
    void RemoveBadRequest();
    static TVector<TLemmaForms> GenerateRealNodes(ELanguage lang, const TUtf16String& lemma, size_t* const formsCount = nullptr);

private:
    static void FlattenLemmas(TWordInstance::TLemmasVector& lms, TLangMask lang, bool bastardsOnly);
    static void GenerateRealNodes(const TLemmaForms& orig, TVector<TLemmaForms>& result);
};

namespace NLemmerAux {
    inline TUtf16String CutWord(const TUtf16String& s) {
        if (s.length() <= MAXWORD_LEN)
            return s;
        return TUtf16String(s.c_str(), MAXWORD_LEN);
    }

    inline bool CmpCutWords(const TUtf16String& s1, const TUtf16String& s2) {
        size_t l1 = Min(s1.length(), (size_t)MAXWORD_LEN);
        size_t l2 = Min(s2.length(), (size_t)MAXWORD_LEN);
        return l1 == l2 && !memcmp(s1.c_str(), s2.c_str(), l1 * sizeof(wchar16));
    }
}
