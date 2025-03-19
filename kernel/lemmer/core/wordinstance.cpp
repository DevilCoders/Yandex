#include <kernel/lemmer/alpha/abc.h>
#include <util/generic/algorithm.h>

#include "lemmeraux.h"
#include "wordinstance.h"

using NLemmerAux::LowCase;
using NLemmerAux::NormalizeForm;

///////////////////////////////
//      TWordInstance
///////////////////////////////

bool TWordInstance::IsBastard(ui32 qual, const TLangMask& lang) const {
    bool res = true;
    for (size_t j = 0; j < Lemmas.size(); ++j)
        if (lang.none() && Lemmas[j].GetLanguage() == LANG_UNK || lang.SafeTest(Lemmas[j].GetLanguage()))
            res = res && Lemmas[j].IsBastard(qual);
    return res;
}

TWordInstance::TWordInstance()
    : TokenCount(1)
    , LangMask()
    , CaseFlags(0)
{
    Lemmas.reserve(MAX_LEMMS);
}

void TWordInstance::Clear() {
    Form.clear();
    NormalizedForm.clear();
    TokenCount = 1;
    Lemmas.clear();
    RedundantLemmas.clear();
    LangMask.Reset();
    CaseFlags = 0;

    Lemmas.reserve(MAX_LEMMS);
}

static void FixCaseFlags(const TWtringBuf& form, TCharCategory& cs) {
    if (form && IsDigit(form[0]))
        cs &= ~CC_TITLECASE;
}

void TWordInstance::SetNormalizedForm(const TUtf16String& form) {
    CaseFlags = NLemmer::ClassifyCase(form.data(), form.size());
    Form = LowCase(form, LANG_UNK);
    NormalizedForm = NormalizeForm(Form, LANG_UNK);
    FixCaseFlags(Form, CaseFlags);
}

bool operator<(const TLemmaForms& a, const TLemmaForms& b) {
    return a.GetLemma() < b.GetLemma();
}

bool TWordInstance::TLemmasVector::AreFormsGenerated() const {
    for (const auto& lemmaForms : *this) {
        if (!lemmaForms.FormsGenerated())
            return false;
    }
    return true;
}

void TWordInstance::TLemmasVector::DiscardForms() {
    for (auto& lemmaForms : *this)
        lemmaForms.DiscardForms();
}

void TWordInstance::AddPlainLemma(const TUtf16String& lemma, ELanguage lang, bool stopWord) {
    if (!Form)
        SetNormalizedForm(lemma);
    LangMask.SafeSet(lang);
    Lemmas.push_back(TLemmaForms(Form, lemma, lang, false));
    if (stopWord)
        Lemmas.back().SetStopWord();
}

TLemmaForms& TWordInstance::AddLemma(const TLemmaForms& lm) {
    if (!Form)
        SetNormalizedForm(lm.GetNormalizedForm());
    LangMask.SafeSet(lm.GetLanguage());
    Lemmas.push_back(lm);
    return Lemmas.back();
}

void TWordInstance::AddLemma(const TYandexLemma& lemma, const TLanguageContext& langInfo, TFormType formType, bool generateForms) {
    LangMask.SafeSet(lemma.GetLanguage());
    Lemmas.push_back(TLemmaForms(lemma, formType, generateForms, &langInfo.GetDecimator()));
    if (lemma.GetLanguage() != LANG_UNK)
        Lemmas.back().StopWord = langInfo.GetStopWords().IsStopWord(Lemmas.back().GetNormalizedForm(), TLangMask(lemma.GetLanguage()), &(Lemmas.back().Stickiness));
}

void TWordInstance::InitLemmas(const TUtf16String& word, const TCharSpan& span, const TLanguageContext& lang, TFormType formType, bool generateForms, bool useFixList) {
    NLemmer::TAnalyzeWordOpt opt("", NLemmer::SfxOnly, NLemmer::MtnSplitAllPossible, NLemmer::AccFoundling);
    opt.UseFixList = useFixList;
    //на время перелемматизации (когда в индексе часть документов хранится с одной версией морфологии, часть - с другой) в визарде нужны формы из обеих версий, чтобы не потерять хиты
    opt.AllowDeprecated = lang.AllowDeprecated;
    opt.AcceptTranslit |= lang.GetTranslitLanguages();
    TTokenStructure tokens;
    tokens.push_back(span);

    TWLemmaArray lemmata;
    NLemmer::AnalyzeWord(TWideToken(word.data(), word.size(), tokens),
                         lemmata, lang.GetLangMask(), lang.GetLangOrder(), opt);

    for (auto& lemma : lemmata) {
        if (lang.GetDisabledLanguages().SafeTest(lemma.GetLanguage())) {
            NLemmerAux::TYandexLemmaSetter setter(lemma);
            setter.SetQuality(TYandexLemma::QFoundling | TYandexLemma::QDisabled | (lemma.GetQuality() & TYandexLemma::QFix));
        }
    }

    lang.GetDecimator().DecimateLemmas(lemmata);

    for (size_t i = 0; i < lemmata.size(); ++i) {
        const TYandexLemma& lemma = lemmata[i];

        AddLemma(lemma, lang, formType, generateForms);
        if (i == 0) { // take the form from the first entry
            Form = LowCase(TUtf16String(lemma.GetInitialForm(), lemma.GetInitialFormLength()), LANG_UNK);
            NormalizedForm = NormalizeForm(Form, LANG_UNK);
            CaseFlags = lemma.GetCaseFlags();
        } else {
            CaseFlags |= lemma.GetCaseFlags();
        }
    }
    FixCaseFlags(Form, CaseFlags);
}

void TWordInstance::Init(const TUtf16String& word, const TCharSpan& span, const TLanguageContext& langInfo, TFormType formType, bool generateForms, bool useFixList) {
    Clear();
    SetNormalizedForm(word);
    TokenCount = 1;

    InitLemmas(word, span, langInfo, formType, generateForms, useFixList);
}

void TWordInstance::Init(const TVector<const TYandexLemma*>& lemmas, const TLanguageContext& langInfo, TFormType formType, bool generateForms) {
    Clear();
    if (lemmas.empty())
        return;

    Form = LowCase(TUtf16String(lemmas[0]->GetInitialForm(), lemmas[0]->GetInitialFormLength()), LANG_UNK);
    NormalizedForm = NormalizeForm(Form, LANG_UNK);

    CaseFlags = lemmas[0]->GetCaseFlags();

    for (auto lemma : lemmas) {
        AddLemma(*lemma, langInfo, formType, generateForms);
        CaseFlags |= lemma->GetCaseFlags();

        if (lemma->GetTokenSpan() > TokenCount)
            TokenCount = lemma->GetTokenSpan();
    }
    FixCaseFlags(Form, CaseFlags);
}

static void GenerateForms(TWordInstance::TLemmasVector& v, const TFormDecimator* decimator) {
    for (auto& i : v) {
        i.ObtainForms();
        if (decimator != i.GetDecimator())
            decimator->DecimateForms(i);
    }
}

void TWordInstance::GenerateAllForms(const TFormDecimator* decimator) {
    ::GenerateForms(Lemmas, decimator);
    ::GenerateForms(RedundantLemmas, decimator);
}

void TWordInstance::CleanBestFlag() {
    for (auto& i : GetLemmas())
        i.SetBest(false);
}

void TWordInstance::SetStopWord(bool stopWord, EStickySide stickiness) {
    for (auto& lemma : Lemmas)
        lemma.SetStopWord(stopWord, stickiness);
}

void TWordInstance::SetStopWord(bool stopWord) {
    for (auto& lemma : Lemmas)
        lemma.SetStopWord(stopWord);
}

bool TWordInstance::IsStopWord(EStickySide& stickiness) const {
    bool stopWord = false;
    stickiness = STICK_NONE;
    for (const auto& lemma : Lemmas) {
        if (lemma.IsStopWord()) {
            stopWord = true;
            stickiness = static_cast<EStickySide>(stickiness | lemma.GetStickiness());
        }
    }
    return stopWord;
}

bool TWordInstance::IsStopWord() const {
    EStickySide stickiness = STICK_NONE;
    return IsStopWord(stickiness);
}

void TWordInstance::ReduntLemma(size_t j) {
    Y_ASSERT(j < Lemmas.size());
    TLemmaForms& lm = Lemmas[j];
    if ((!lm.Forms || lm.HasExactForm()) && lm.SetFormType(fExactWord) != TLemmaForms::sftMustBeDeleted) {
        lm.SetBest(false);
        RedundantLemmas.push_back(lm);
    }
    Lemmas.erase(j);
}

TLemmaForms::TSetFormtypeRes TWordInstance::SetFormTypeOrErase(size_t lemmNum, TFormType newType) {
    TLemmaForms::TSetFormtypeRes res = Lemmas[lemmNum].SetFormType(newType);
    if (res == TLemmaForms::sftMustBeDeleted)
        Lemmas.erase(lemmNum);
    return res;
}

void TWordInstance::SetNonLemmerFormType(TFormType formType) {
    for (size_t i = Lemmas.size(); i > 0; --i)
        Lemmas[i - 1].FormType = formType;
}

bool TWordInstance::SetFormType(TFormType newType) {
#if !defined(NDEBUG) && !defined(__GCCXML__)
    bool flAny = !Lemmas.empty();
#endif

    bool ret = true;
    for (size_t i = Lemmas.size(); i > 0; --i) {
        if (SetFormTypeOrErase(i - 1, newType) == TLemmaForms::sftFailed)
            ret = false;
    }

#if !defined(NDEBUG) && !defined(__GCCXML__)
    Y_ASSERT(!flAny || !Lemmas.empty());
#endif

    return ret;
}

static inline bool LessFormType(const TFormType x, const TFormType y) {
    if (x == fExactWord && y == fExactLemma)
        return false;
    else if (y == fExactWord && x == fExactLemma)
        return true;
    return (int)x < (int)y;
}

TFormType TWordInstance::GetFormType() const {
    TFormType ret = fGeneral;
    TLemmasVector::const_iterator i = Lemmas.begin();
    if (i != Lemmas.end())
        ret = (i++)->GetFormType();
    for (; i != Lemmas.end(); ++i) {
        if (LessFormType(i->GetFormType(), ret))
            ret = i->GetFormType();
    }
    return ret;
}
