#include <kernel/search_types/search_types.h>
#include <kernel/lemmer/alpha/abc.h>
#include <util/generic/algorithm.h>

#include "language.h"
#include "lemmeraux.h"

#include "lemmaforms.h"

namespace {
    template <class TChecker>
    bool CheckStemGrammar(const TLemmaForms& lf, TChecker checker) {
        return checker(lf.GetStemGrammar());
    }

    template <class TChecker>
    bool CheckFlexGrammar(const TLemmaForms& lf, TChecker checker) {
        for (size_t i = 0; i < lf.GetFlexGrammars().size(); i++) {
            if (checker(lf.GetFlexGrammars()[i]))
                return true;
        }
        return false;
    }

    struct THasAll {
        const TGramBitSet Gr;
        THasAll(const TGramBitSet& gr)
            : Gr(gr)
        {
        }
        bool operator()(const TGramBitSet& gr) const {
            return (gr & Gr) == Gr;
        }
    };

    struct THasAny {
        const TGramBitSet Gr;
        THasAny(const TGramBitSet& gr)
            : Gr(gr)
        {
        }
        bool operator()(const TGramBitSet& gr) const {
            return (gr & Gr).any();
        }
    };

    struct THas {
        const EGrammar Gr;
        THas(EGrammar gr)
            : Gr(gr)
        {
        }
        bool operator()(const TGramBitSet& gr) const {
            return gr.Test(Gr);
        }
    };

    bool IsIntrusiveBastard(const TYandexLemma& lemma, bool useFixList) {
        if (!lemma.IsBastard() || lemma.GetLanguage() == LANG_UNK)
            return false; // for safety

        TWLemmaArray out;
        out.reserve(MAX_LEMMS);
        NLemmer::TRecognizeOpt opt(NLemmer::AccDictionary);
        opt.UseFixList = useFixList;
        size_t n = NLemmer::GetLanguageById(lemma.GetLanguage())->Recognize(lemma.GetText(), lemma.GetTextLength(), out, opt);
        for (size_t i = 0; i < n; ++i) {
            if (lemma.GetTextBuf() == out[i].GetTextBuf())
                return true;
        }
        return false;
    }
}

namespace NLemmerAux {
    TUtf16String LowCase(const TUtf16String& form, ELanguage lang) {
        const NLemmer::TAlphabetWordNormalizer* awn = NLemmer::GetAlphaRules(lang);
        TUtf16String ret = form;
        awn->ToLower(ret);
        return ret;
    }

    TUtf16String NormalizeForm(const TUtf16String& form, ELanguage lang, bool derenyx) {
        const NLemmer::TAlphabetWordNormalizer* awn = NLemmer::GetAlphaRules(lang);
        TUtf16String ret = form;
        NLemmer::TConvertMode mode(NLemmer::CnvDecompose, NLemmer::CnvCompose, NLemmer::CnvLowCase);
        if (derenyx)
            mode.Set(NLemmer::CnvDerenyx);
        awn->Normalize(ret, mode);
        return ret;
    }

}
using NLemmerAux::LowCase;
using NLemmerAux::NormalizeForm;

///////////////////////////////
//      TLemmaForms
///////////////////////////////

TLemmaForms::TLemmaForms(const TUtf16String& form, const TUtf16String& lemma, ELanguage lang, bool isExact, ui32 quality, TFormType fType, const TGramBitSet& stemGrammar)
    : Form(LowCase(form, lang))
    , NormalizedForm(NormalizeForm(Form, lang))
    , FormType(fType)
    , Lemma(lemma)
    , Language(lang)
    , ExactLemma(isExact ? NLemmer::Lll : NLemmer::LllNo)
    , Best(isExact)
    , Quality(quality)
    , StopWord(false)
    , Stickiness(STICK_NONE)
    , Overridden(false)
    , StemGrammar(stemGrammar)
    , Forms(new TFormMap)
    , Decimator(nullptr)
{
}

TLemmaForms::TLemmaForms(const TYandexLemma& lemma, TFormType fType, bool generateForms, const TFormDecimator* decimator)
    : YandexLemma(new TYandexLemma(lemma))
    , Form(LowCase(TUtf16String(YandexLemma->GetInitialForm(), YandexLemma->GetInitialFormLength()), YandexLemma->GetLanguage()))
    , NormalizedForm(YandexLemma->GetNormalizedForm(), YandexLemma->GetNormalizedFormLength())
    , FormType(fType)
    , Lemma(YandexLemma->GetText(), YandexLemma->GetTextLength())
    , Language(YandexLemma->GetLanguage())
    , ExactLemma(YandexLemma->LooksLikeLemma())
    , Best(ExactLemma)
    , Quality(YandexLemma->GetQuality())
    , StopWord(false)
    , Stickiness(STICK_NONE)
    , Overridden(false)
    , StemGrammar(TGramBitSet::FromBytes(YandexLemma->GetStemGram()))
    , Decimator(decimator)
{
    TGramBitSet add = TGramBitSet::FromBytes(YandexLemma->GetAddGram()) & ~StemGrammar;
    FlexGrammars.reserve(YandexLemma->FlexGramNum());
    for (size_t i = 0; i < lemma.FlexGramNum(); i++)
        FlexGrammars.push_back(TGramBitSet::FromBytes(lemma.GetFlexGram()[i]) | add);
    if (generateForms)
        GenerateForms();
}

TLemmaForms::TLemmaForms(const TLemmaForms::TFormMap::value_type& form, const TLemmaForms& lemma)
    : Form(form.first)
    , NormalizedForm(Form)
    , FormType(lemma.GetFormType())
    , Lemma(Form)
    , Language(lemma.GetLanguage())
    , ExactLemma(lemma.IsBest() && form.second.IsExact ? NLemmer::Lll : NLemmer::LllNo)
    , Best(ExactLemma)
    , Quality(lemma.GetQuality())
    , StopWord(lemma.IsStopWord())
    , Stickiness(lemma.GetStickiness())
    , Overridden(true)
    , StemGrammar()
    , Forms(new TFormMap)
{
    NLemmer::GetAlphaRules(Language)->Derenyx(NormalizedForm);

    // we expect, that all forms are 'normal'
    // if not - we'll not find it in initial lemma
    // so we'll not find it in this one
    Y_ASSERT(NormalizeForm(NormalizedForm, Language) == NormalizedForm);
    Y_ASSERT(NormalizeForm(Lemma, LANG_UNK) == Lemma);

    AddFormaMergeInt(form.first, form.second);
}

inline void TLemmaForms::AddFormaMergeInt(const TUtf16String& form, const TFormWeight& weight, const NSpike::TGrammarBunch& grammems) {
    TFormWeight& w = AddFormaMergeInt(form, weight);
    if (w.Grams.empty())
        w.Grams = grammems;
}

static void InitFormGrammems(const TYandexLemma& lemma, NSpike::TGrammarBunch& formgrams) {
    if (formgrams.empty()) {
        if (lemma.FlexGramNum() > 0)
            NSpike::ToGrammarBunch(lemma.GetStemGram(), lemma.GetFlexGram(), lemma.FlexGramNum(), formgrams);
        else
            NSpike::ToGrammarBunch(lemma.GetStemGram(), formgrams);
        NSpike::Normalize(&formgrams, lemma.GetLanguage());
    }
}

void TLemmaForms::AddExactForms(const TLanguage* lang) {
    TLemmaForms::TFormWeight* weight = &AddFormaMergeInt(NormalizeForm(Form, Language, false),
                                                         TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
    InitFormGrammems(*YandexLemma.Get(), weight->Grams);

    const NSpike::TGrammarBunch& gr = weight->Grams;
    AddFormaMergeInt(NormalizedForm, TLemmaForms::TFormWeight(TLemmaForms::ExactMatch), gr);

    TUtf16String s = NormalizedForm;
    lang->AlphaRules().Convert(s);
    AddFormaMergeInt(s, TLemmaForms::TFormWeight(TLemmaForms::ExactMatch), gr);

    if (FormType != fExactWord && FormType != fWeirdExactWord) {
        wchar16 formBuf[MAXWORD_BUF];
        using NLemmer::TAlphabetWordNormalizer;
        size_t formLen = lang->Normalize(Form.data(), Form.size(), formBuf, MAXWORD_LEN).Length;
        AddFormaMergeInt(TUtf16String(formBuf, formLen), TLemmaForms::TFormWeight(TLemmaForms::InexactMatch), gr);
    }
}

void TLemmaForms::GenerateForms() {
    Y_ASSERT(YandexLemma);
    if (!YandexLemma)
        return;

    if (!Forms)
        Forms.Reset(new TFormMap);
    const TLanguage* lang = NLemmer::GetLanguageById(YandexLemma->GetLanguage());

    AddExactForms(lang);

    TAutoPtr<NLemmer::TGrammarFiltr> lemmaFiltr;
    if (FormType == fExactLemma) {
        lemmaFiltr = YandexLemma->LooksLikeLemmaExt();
        if (!lemmaFiltr.Get())
            return;
    }
    if (Decimator && Decimator->IsApplicable(*YandexLemma))
        lemmaFiltr.Reset(new TFormDecimatorFilter(Decimator, lang->Id, lemmaFiltr.Release()));

    if (FormType != fExactWord && FormType != fWeirdExactWord) {
        TAutoPtr<NLemmer::TFormGenerator> gen = YandexLemma->Generator(lemmaFiltr.Get());
        TYandexWordform form;
        while (gen->GenerateNext(form)) {
            TLemmaForms::TFormWeight& weight = AddFormaMergeInt(form.GetText(), TLemmaForms::TFormWeight(TLemmaForms::InexactMatch));
            if (!weight.Grams.empty())
                continue;
            if (form.FlexGramNum() > 0)
                NSpike::ToGrammarBunch(YandexLemma->GetStemGram(), form.GetFlexGram(), form.FlexGramNum(), weight.Grams);
            else
                NSpike::ToGrammarBunch(YandexLemma->GetStemGram(), weight.Grams);
            NSpike::Normalize(&weight.Grams, YandexLemma->GetLanguage());
        }
    }
}

void TLemmaForms::RestrictFormsByLemma() {
    Y_ASSERT(YandexLemma);
    if (!YandexLemma)
        return;

    Y_ASSERT(FormType != fGeneral);
    Y_ASSERT(Forms);
    TSet<TUtf16String> keep;

    if (FormType != fExactWord && FormType != fWeirdExactWord) {
        const TLanguage* lang = NLemmer::GetLanguageById(YandexLemma->GetLanguage());
        {
            wchar16 formBuf[MAXWORD_BUF];
            using NLemmer::TAlphabetWordNormalizer;
            size_t formLen = lang->Normalize(Form.data(), Form.size(), formBuf, MAXWORD_LEN).Length;
            keep.insert(TUtf16String(formBuf, formLen));
        }
        Y_ASSERT(FormType == fExactLemma);
        TAutoPtr<NLemmer::TGrammarFiltr> exactLemmaFiltr(YandexLemma->LooksLikeLemmaExt());
        if (exactLemmaFiltr) {
            TAutoPtr<NLemmer::TFormGenerator> gen = YandexLemma->Generator(exactLemmaFiltr.Get());
            TYandexWordform form;
            while (gen->GenerateNext(form))
                keep.insert(form.GetText());
        }
    }
    TFormMap::iterator it = Forms.Mutable()->begin();
    while (it != Forms.Mutable()->end()) {
        TSet<TUtf16String>::const_iterator i = keep.find(it->first);
        if (!it->second.IsExact && i == keep.end())
            Forms.Mutable()->erase(it++);
        else
            ++it;
    }
}

void TLemmaForms::RestrictFormsNoLemma() {
    Y_ASSERT(FormType != fGeneral);
    Y_ASSERT(Forms);
    if (FormType != fExactWord && FormType != fWeirdExactWord && !(FormType == fExactLemma && IsExactLemma())) {
        return;
    }
    TFormMap::iterator it = Forms.Mutable()->begin();
    while (it != Forms.Mutable()->end()) {
        if (!it->second.IsExact)
            Forms.Mutable()->erase(it++);
        else
            ++it;
    }
}

TLemmaForms::TSetFormtypeRes TLemmaForms::SetFormType(TFormType newType) {
    if (newType == fWeirdExactWord && FormType == fExactWord)
        return sftFailed;
    if (newType == FormType)
        return sftOk;
    if (newType == fExactWord || newType == fWeirdExactWord || newType == fExactLemma && FormType == fGeneral) {
        FormType = newType;
        if (!Forms)
            return sftOk;
        bool flEmpty = !Forms || Forms->empty();

        if (YandexLemma)
            RestrictFormsByLemma();
        else
            RestrictFormsNoLemma();

        if (!flEmpty && Forms->empty())
            return sftMustBeDeleted;
    } else if (newType == fGeneral || newType == fExactLemma && (FormType == fExactWord || FormType == fWeirdExactWord)) {
        if (!YandexLemma)
            return sftFailed;
        FormType = newType;
        if (Forms && !Forms->empty())
            GenerateForms();
    }
    return sftOk;
}

bool TLemmaForms::HasExactForm() const {
    Y_ASSERT(Forms);
    return Forms->find(NormalizedForm) != Forms->end();
}

bool TLemmaForms::IsIntrusiveBastard(bool useFixList) const {
    return YandexLemma && ::IsIntrusiveBastard(*YandexLemma.Get(), useFixList);
}

void TLemmaForms::SetStopWord(bool stopWord, EStickySide stickiness) {
    // TODO: Uncomment after wizard release and qtrees update
    //Y_ASSERT(stopWord || stickiness == STICK_NONE);
    SetStopWord(stopWord);
    Stickiness = stickiness;
}

void TLemmaForms::SetStopWord(bool stopWord) {
    StopWord = stopWord;
    if (!StopWord) {
        Stickiness = STICK_NONE;
    }
}

bool TLemmaForms::IsBastard(ui32 qual /*= TYandexLemma::QAnyBastard*/) const {
    return Quality & qual;
}

void TLemmaForms::Swap(TLemmaForms& ot) {
    using std::swap;
    DoSwap(YandexLemma, ot.YandexLemma);
    Form.swap(ot.Form);
    NormalizedForm.swap(ot.NormalizedForm);
    swap(FormType, ot.FormType);
    Lemma.swap(ot.Lemma);
    swap(Language, ot.Language);

    DoSwap(StemGrammar, ot.StemGrammar);
    FlexGrammars.swap(ot.FlexGrammars);

    swap(ExactLemma, ot.ExactLemma);
    swap(Best, ot.Best);
    swap(Quality, ot.Quality);
    swap(StopWord, ot.StopWord);
    swap(Stickiness, ot.Stickiness);
    swap(Overridden, ot.Overridden);

    Forms.Swap(ot.Forms);

    swap(Decimator, ot.Decimator);
}

bool TLemmaForms::HasWholeStemGram(const TGramBitSet& gr) const {
    return CheckStemGrammar<THasAll>(*this, gr);
}
bool TLemmaForms::HasWholeFlexGram(const TGramBitSet& gr) const {
    return CheckFlexGrammar<THasAll>(*this, gr);
}
bool TLemmaForms::HasWholeGram(const TGramBitSet& gr) const {
    TGramBitSet g = gr & ~GetStemGrammar();
    if (g.none())
        return true;
    return HasWholeFlexGram(g);
}

bool TLemmaForms::HasStemGram(EGrammar gr) const {
    return CheckStemGrammar<THas>(*this, gr);
}
bool TLemmaForms::HasFlexGram(EGrammar gr) const {
    return CheckFlexGrammar<THas>(*this, gr);
}
bool TLemmaForms::HasGram(EGrammar gr) const {
    return HasStemGram(gr) || HasFlexGram(gr);
}

bool TLemmaForms::HasAnyStemGram(const TGramBitSet& gr) const {
    return CheckStemGrammar<THasAny>(*this, gr);
}
bool TLemmaForms::HasAnyFlexGram(const TGramBitSet& gr) const {
    return CheckFlexGrammar<THasAny>(*this, gr);
}
bool TLemmaForms::HasAnyGram(const TGramBitSet& gr) const {
    return HasAnyStemGram(gr) || HasAnyFlexGram(gr);
}

TLemmaForms TLemmaForms::MakeExactDefaultLemmaForms(const TUtf16String& text) {
    TLemmaForms result(text, text, LANG_UNK);
    result.AddFormaMerge(text, TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
    return result;
}
