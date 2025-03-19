#include "morphfixlist.h"
#include "wordinstance.h"
#include "lemmeraux.h"
#include <library/cpp/token/charfilter.h>

namespace NWordInstanceSelectBestGetQual {
    enum TQual { qNo = 0,
                 qFoundling,
                 qBastard,
                 qGood };
    TQual getQuality(ui32 q) {
        if (q & (TYandexLemma::QBastard | TYandexLemma::QSob))
            return qBastard;
        if (q & TYandexLemma::QFoundling)
            return qFoundling;
        return qGood;
    }
}

TLangMask TWordInstanceUpdate::SelectBest(bool removeBad, TLangMask preferableLanguage) {
    using namespace NWordInstanceSelectBestGetQual;
    TQual theBest = qNo;
    TQual thePrefBest = qNo;
    TQual theWorst = qNo;

    bool hasBest[LANG_MAX] = {false};

    for (auto& lemma : Wi.Lemmas) {
        TQual q = getQuality(lemma.GetQuality());
        if (lemma.IsBest()) {
            if (q > theBest)
                theBest = q;
            if (preferableLanguage.SafeTest(lemma.GetLanguage()) && q > thePrefBest) // dirty hack
                thePrefBest = q;
            hasBest[lemma.GetLanguage()] = true;
        } else {
            if (q == qNo || q < theWorst)
                theWorst = q;
        }
    }
    if (theBest < theWorst)
        return TLangMask();

    TLangMask toKill;
    TLangMask toKillNot;
    for (size_t j = Wi.Lemmas.size(); j > 0; --j) {
        TQual q = getQuality(Wi.Lemmas[j - 1].GetQuality());
        ELanguage lg = Wi.Lemmas[j - 1].GetLanguage();
        bool b = hasBest[lg];
        if (removeBad && !Wi.Lemmas[j - 1].IsBest() && b)
            Wi.ReduntLemma(j - 1);

        TQual bq = preferableLanguage.SafeTest(lg) ? thePrefBest : theBest;
        if (!b && q <= bq || q < bq)
            toKill.SafeSet(lg);
        else
            toKillNot.SafeSet(lg);
    }

    return toKill & ~toKillNot;
}

template <TYandexLemma::TQuality bastQual>
static bool CheckQual_(bool bastardsOnly, const TLemmaForms& lm) {
    return !bastardsOnly || (lm.GetQuality() & bastQual);
}

static bool CheckLang_(const TLangMask& lang, const TLemmaForms& lm) {
    return lang.SafeTest(lm.GetLanguage());
}

template <TYandexLemma::TQuality bastQual>
static bool CheckLangQual(const TLangMask& lang, bool bastardsOnly, const TLemmaForms& lm) {
    return CheckLang_(lang, lm) && CheckQual_<bastQual>(bastardsOnly, lm);
}

void TWordInstanceUpdate::FlattenLemmas(TWordInstance::TLemmasVector& lms, TLangMask lang, bool bastardsOnly) {
    static const TYandexLemma::TQuality BastQual =
        static_cast<TYandexLemma::TQuality>(TYandexLemma::QBastard | TYandexLemma::QSob);

    size_t numForms = 0;
    for (size_t i = lms.size(); i > 0; --i) {
        const TLemmaForms& lm = lms[i - 1];
        if (CheckLangQual<BastQual>(lang, bastardsOnly, lm))
            numForms += lm.GetForms().size();
    }
    if (!numForms)
        return;

    TVector<TLemmaForms> newLemmas;
    newLemmas.reserve(numForms);
    for (size_t i = lms.size(); i > 0; --i) {
        const TLemmaForms& lm = lms[i - 1];
        if (!CheckLangQual<BastQual>(lang, bastardsOnly, lm))
            continue;

        for (TLemmaForms::TFormMap::const_iterator it = lm.GetForms().begin(); it != lm.GetForms().end(); ++it) {
            newLemmas.push_back(TLemmaForms(*it, lm));
        }
    }

    typedef TMultiMap<TUtf16String, TLemmaForms*> TSorterType;
    TSorterType sorter;

    // Insert existing items
    for (size_t i = 0; i < lms.size(); ++i) {
        TLemmaForms& lm = lms[i];
        bool isBastard = (lm.GetQuality() & (TYandexLemma::QBastard | TYandexLemma::QSob));
        if (!lang.SafeTest(lm.GetLanguage()) || (bastardsOnly && !isBastard)) {
            sorter.insert(sorter.end(), TSorterType::value_type(lm.GetLemma(), &lm));
        }
    }

    // Insert new items, trying to preserve the order
    for (size_t i = 0; i < newLemmas.size(); ++i) {
        TLemmaForms& lm = newLemmas[i];
        const TUtf16String& lemma = lm.GetLemma();
        sorter.insert(sorter.upper_bound(lemma), TSorterType::value_type(lemma, &lm));
    }

    // Move the whole thing to the new array, destroying previous locations
    TVector<TLemmaForms> resultingLemmas;
    resultingLemmas.resize(sorter.size());

    size_t rcount = 0;
    for (TSorterType::iterator it = sorter.begin(); it != sorter.end(); ++it, ++rcount) {
        resultingLemmas[rcount].Swap(*(it->second));
    }

    // Move the result to the original array and obliviate its previous contents
    lms.swap(resultingLemmas);
}

void TWordInstanceUpdate::FlattenLemmas(TLangMask lang, bool bastardsOnly) {
    FlattenLemmas(Wi.Lemmas, lang, bastardsOnly);
    FlattenLemmas(Wi.RedundantLemmas, lang, bastardsOnly);
}

static size_t Exist(const TVector<TLemmaForms>& vl, ELanguage lang) {
    size_t i = 0;
    for (; i < vl.size(); ++i) {
        if (vl[i].GetLanguage() == lang)
            break;
    }
    return i;
}

static void ShrinkLemmas(TWordInstance::TLemmasVector& lms, const TUtf16String& form, TLangMask lang, bool bastardsOnly) {
    using NLemmerAux::NormalizeForm;
    const TUtf16String formNormalizedU = NormalizeForm(form, LANG_UNK, false);
    TVector<TLemmaForms> newLemmas;
    for (size_t i = lms.size(); i > 0; --i) {
        const TLemmaForms& lm = lms[i - 1];
        if (!CheckLangQual<TYandexLemma::QBastard>(lang, bastardsOnly, lm))
            continue;
        if (lm.GetQuality() & TYandexLemma::QAutomorphology)
            continue;

        size_t pls = Exist(newLemmas, lm.GetLanguage());
        if (pls < newLemmas.size()) {
            newLemmas[pls].SetBest(newLemmas[pls].IsBest() || lm.IsBest());

            if (!newLemmas[pls].IsStopWord())
                newLemmas[pls].SetStopWord(lm.IsStopWord(), lm.GetStickiness());
        } else {
            Y_ASSERT(pls == newLemmas.size());
            newLemmas.push_back(TLemmaForms(form, formNormalizedU, lm.GetLanguage(),
                                            lm.IsBest(), lm.GetQuality(), lm.GetFormType()));
            newLemmas.back().SetStopWord(lm.IsStopWord(), lm.GetStickiness());
        }
        Y_ASSERT(pls < newLemmas.size());
        newLemmas[pls].AddFormaMerge(NormalizeForm(form, lm.GetLanguage(), false), TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
        newLemmas[pls].AddFormaMerge(NormalizeForm(lm.GetInitialForm(), lm.GetLanguage(), false), TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
        lms.erase(i - 1);
    }
    for (size_t i = 0; i < newLemmas.size(); ++i)
        lms.push_back(newLemmas[i]);
}

void TWordInstanceUpdate::ShrinkLemmas(TLangMask lang, bool bastardsOnly) {
    TUtf16String cuttedNormalizedForm = NLemmerAux::CutWord(Wi.GetNormalizedForm());
    ::ShrinkLemmas(Wi.Lemmas, cuttedNormalizedForm, lang, bastardsOnly);
    ::ShrinkLemmas(Wi.RedundantLemmas, cuttedNormalizedForm, lang, bastardsOnly);
}

void TWordInstanceUpdate::RepairLLL(bool removeOdd) {
    typedef TEnumBitSet<ELanguage, LANG_UNK, LANG_MAX> TLangMaskEx;
    TLangMaskEx toAdd;
    TLangMaskEx notToAdd;
    TLangMaskEx toSanitize;
    for (size_t i = Wi.Lemmas.size(); i > 0; --i) {
        if (Wi.Lemmas[i - 1].IsExactLemma()) {
            notToAdd.SafeSet(Wi.Lemmas[i - 1].GetLanguage());
            continue;
        }
        if (Wi.Lemmas[i - 1].GetFormType() != fExactLemma)
            continue;
        toSanitize.SafeSet(Wi.Lemmas[i - 1].GetLanguage());
        toAdd.SafeSet(Wi.Lemmas[i - 1].GetLanguage());
        Wi.SetFormTypeOrErase(i - 1, fExactWord);
    }
    toSanitize &= notToAdd;
    if (removeOdd && toSanitize.any()) {
        for (size_t i = Wi.Lemmas.size(); i > 0; --i) {
            if (!Wi.Lemmas[i - 1].IsExactLemma() && toSanitize.SafeTest(Wi.Lemmas[i - 1].GetLanguage()))
                Wi.Lemmas.erase(i - 1);
        }
    }

    toAdd &= ~notToAdd;
    for (ELanguage l : toAdd)
        Wi.Lemmas.push_back(TLemmaForms(Wi.Form, Wi.NormalizedForm, l, false, TYandexLemma::QFoundling, fExactLemma));
}

void TWordInstanceUpdate::RemoveBadRequest() {
    TWordInstance::TLemmasVector& lemmas = Wi.GetLemmas();
    TLangMask hasBadBest;
    TLangMask hasGoodBest;

    for (const auto& lemma : lemmas) {
        if (lemma.IsExactLemma()) {
            if (lemma.IsBadRequest())
                hasBadBest.SafeSet(lemma.GetLanguage());
            else
                hasGoodBest.SafeSet(lemma.GetLanguage());
        }
    }

    hasBadBest &= ~hasGoodBest;

    for (size_t i = lemmas.size(); i > 0; --i) {
        if (lemmas[i - 1].IsBadRequest() && !(lemmas[i - 1].IsExactLemma() && hasBadBest.SafeTest(lemmas[i - 1].GetLanguage()))) {
            Wi.ReduntLemma(i - 1);
        }
    }
    Y_ASSERT(!Wi.GetLemmas().empty());
}

void TWordInstanceUpdate::ShrinkIntrusiveBastards(bool useFixList) {
    TWordInstance::TLemmasVector& lemmas = Wi.GetLemmas();
    TLangMask hasIntrusiveBastards;

    for (const auto& lemma : lemmas) {
        if (lemma.IsIntrusiveBastard(useFixList))
            hasIntrusiveBastards.SafeSet(lemma.GetLanguage());
    }
    if (!hasIntrusiveBastards.any())
        return;
    for (size_t i = lemmas.size(); i > 0; --i) {
        if (hasIntrusiveBastards.SafeTest(lemmas[i - 1].GetLanguage()))
            Wi.SetFormTypeOrErase(i - 1, fWeirdExactWord);
    }
    Y_ASSERT(!Wi.GetLemmas().empty());
}

namespace {
    bool HasLemmaAndForm(const TWordInstance::TLemmasVector& v, const TUtf16String& lemma, const TUtf16String& form) {
        for (const auto& i : v) {
            if (i.GetLemma() == lemma && i.HasForm(form))
                return true;
        }
        return false;
    }
    static bool HasLemmaAndLang(const TWordInstance::TLemmasVector& v, const TUtf16String& w, ELanguage lang) {
        for (const auto& i : v) {
            if (i.GetLemma() == w && i.GetLanguage() == lang)
                return true;
        }
        return false;
    }
    static bool HasLemma(const TWordInstance::TLemmasVector& v, const TUtf16String& w) {
        for (const auto& i : v) {
            if (i.GetLemma() == w)
                return true;
        }
        return false;
    }

    static void InsertForma(TWordInstance::TLemmasVector& v, TUtf16String form, const TUtf16String& lmText, ELanguage lang) {
        if (HasLemmaAndLang(v, lmText, lang))
            return;
        form = NLemmerAux::CutWord(form);
        TLemmaForms lm(form, lmText, lang);
        lm.AddFormaMerge(form, TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
        v.push_back(lm);
    }

    static void InsertForma(TWordInstance::TLemmasVector& v, TUtf16String form, ELanguage lang) {
        TUtf16String lmText = NLemmerAux::CutWord(NormalizeUnicode(form));
        InsertForma(v, form, lmText, lang);
    }

    static bool SubLangMask(TLangMask& langMask, const TWordInstance::TLemmasVector& lms) {
        for (size_t i = 0; i < lms.size() && langMask.any(); ++i)
            langMask.SafeReset(lms[i].GetLanguage());
        return langMask.none();
    }

    static void AddDefaultLemma(TWordInstance::TLemmasVector& redundantLemmas, const TWordInstance::TLemmasVector& principalLemmas, const TUtf16String& form) {
        if (redundantLemmas.empty() && principalLemmas.empty())
            return;
        InsertForma(redundantLemmas, NormalizeUnicode(form), LANG_UNK);

        TLangMask langMask = NLemmer::ClassifyLanguage(form.data(), form.size()) & NLanguageMasks::AllLanguages();
        if (SubLangMask(langMask, principalLemmas) || SubLangMask(langMask, redundantLemmas))
            return;
        NLemmer::TAnalyzeWordOpt opt = NLemmer::TAnalyzeWordOpt::IndexerOpt();
        opt.Suffix = NLemmer::SfxOnly;
        opt.Multitoken = NLemmer::MtnWholly;
        opt.AcceptFoundling.Reset();
        opt.ReturnFoundlingAnyway = false;
        opt.ResetLemmaToForm = false;
        opt.UseFixList = false;

        TWLemmaArray lms;
        AnalyzeWord(form.data(), form.size(), lms, langMask, nullptr, opt);
        for (size_t i = 0; i < lms.size(); ++i)
            InsertForma(redundantLemmas, form, TUtf16String(lms[i].GetText(), lms[i].GetTextLength()), lms[i].GetLanguage());
    }

    class TRedLemmasVector: public TWordInstance::TLemmasVector {
    private:
        void InsertForma(const TUtf16String& form, const TUtf16String& lmText, const TWordInstance::TLemmasVector& principalLemmas, ELanguage lang) {
            if (HasLemma(*this, lmText))
                return;
            if (HasLemmaAndForm(principalLemmas, lmText, form))
                return;
            TLemmaForms lm(form, lmText, lang);
            lm.AddFormaMerge(form, TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
            push_back(lm);
        }

    public:
        TRedLemmasVector(const TWordInstance::TLemmasVector& redundantLemmas, const TWordInstance::TLemmasVector& principalLemmas) {
            for (size_t i = 0; i < redundantLemmas.size(); ++i) {
                TUtf16String form = NLemmerAux::CutWord(redundantLemmas[i].GetNormalizedForm());
                TUtf16String lmText = NLemmerAux::CutWord(redundantLemmas[i].GetLemma());

                InsertForma(form, lmText, principalLemmas, redundantLemmas[i].GetLanguage());
                InsertForma(form, form, principalLemmas, redundantLemmas[i].GetLanguage());
            }
        }
    };
}

void TWordInstanceUpdate::UpdateRedundantLemmas() {
    TRedLemmasVector redL(Wi.RedundantLemmas, Wi.Lemmas);
    Wi.RedundantLemmas.swap(redL);
}

void TWordInstanceUpdate::InjectRedundantLemmas() {
    Wi.Lemmas.reserve(Wi.Lemmas.size() + Wi.RedundantLemmas.size());
    for (auto& redLemma : Wi.RedundantLemmas) {
        Wi.Lemmas.emplace_back(std::move(redLemma));
    }
    Wi.RedundantLemmas.clear();
}

void TWordInstanceUpdate::AddDefaultLemma() {
    ::AddDefaultLemma(Wi.RedundantLemmas, Wi.Lemmas, Wi.GetNormalizedForm());
}

void TWordInstanceUpdate::GenerateRealNodes(const TLemmaForms& orig, TVector<TLemmaForms>& result) {
    //We need the same TLangauge as in index to generate lemmas for fixlist forms
    const TLanguage* lang = NLemmer::GetLanguageWithMeaningfulLemmasById(orig.GetLanguage());
    if (!lang) {
        if (orig.GetOriginalLemma()) {
            result.push_back(*orig.GetOriginalLemma());
        }
        return;
    }

    //copypasted from analyze_word.cpp
    NLemmer::TAnalyzeWordOpt analyzeOpt("", NLemmer::SfxOnly, NLemmer::MtnSplitAllPossible, NLemmer::AccFoundling);
    analyzeOpt.UseFixList = false;
    NLemmer::TRecognizeOpt recognizeOpt(analyzeOpt, NLemmerAux::QualitySetByMaxLevel(NLemmer::AccFoundling));
    recognizeOpt.SkipValidation = analyzeOpt.AnalyzeWholeMultitoken;
    recognizeOpt.GenerateQuasiBastards = analyzeOpt.GenerateQuasiBastards.SafeTest(orig.GetLanguage());
    lang->TuneOptions(analyzeOpt, recognizeOpt);

    for (const auto& form : orig.GetForms()) {
        TWLemmaArray lms;
        lang->Recognize(form.first.c_str(), form.first.size(), lms, recognizeOpt);
        for (size_t k = 0; k != lms.size(); ++k) {
            TUtf16String lmText(lms[k].GetText(), lms[k].GetTextLength());
            TLemmaForms* lm = nullptr;
            for (size_t i = 0; i != result.size(); ++i) {
                if (result[i].GetLemma() == lmText && result[i].GetLanguage() == orig.GetLanguage()) {
                    lm = &result[i];
                    break;
                }
            }
            if (!lm) {
                TLemmaForms newLF(orig.GetInitialForm(), lmText, orig.GetLanguage(),
                                  lms[k].LooksLikeLemma(), TYandexLemma::QDictionary, orig.GetFormType());
                result.push_back(newLF);
                lm = &result.back();
                lm->Overridden = true;
            }
            lm->AddFormaMerge(form.first, form.second);
            if (orig.IsStopWord())
                lm->SetStopWord(true, orig.GetStickiness());
        }
    }
}

TVector<TLemmaForms> TWordInstanceUpdate::GenerateRealNodes(ELanguage lang, const TUtf16String& form, size_t* const formsCount) {
    TWordInstance wi;
    wi.Init(form, TCharSpan(0, form.size()));
    TVector<TLemmaForms> result;
    if (formsCount)
        *formsCount = 0;
    for (size_t i = wi.Lemmas.size(); i > 0; --i) {
        const TLemmaForms& lf = wi.Lemmas[i - 1];
        if (!(lf.GetQuality() & TYandexLemma::QFix))
            continue;
        if (lf.GetLanguage() != lang)
            continue;
        if (formsCount)
            *formsCount += lf.NumForms();
        GenerateRealNodes(lf, result);
    }
    return result;
}

void TWordInstanceUpdate::SpreadFixListLemmas() {
    THashMap<ELanguage, size_t> totalFormsCount;
    TLangMask processedLangs;
    for (size_t i = 0; i != Wi.Lemmas.size(); ++i) {
        const TLemmaForms& lf = Wi.Lemmas[i];
        if (lf.GetQuality() & TYandexLemma::QFix)
            totalFormsCount[lf.GetLanguage()] += lf.NumForms();
    }
    for (size_t i = Wi.Lemmas.size(); i > 0; --i) {
        const TLemmaForms& lf = Wi.Lemmas[i - 1];
        ELanguage lang = lf.GetLanguage();
        if (!(lf.GetQuality() & TYandexLemma::QFix))
            continue;
        if (!processedLangs.Test(lang)) {
            size_t base = Wi.Lemmas.size();
            const TVector<TLemmaForms>* realNodes = nullptr;

            const NLemmer::TMorphFixList* morphFixList = NLemmer::GetMorphFixList();
            Y_ASSERT(morphFixList);
            if (morphFixList)
                realNodes = morphFixList->GetRealNodes(lang, Wi.GetInitialForm(), totalFormsCount[lang]);

            if (realNodes) {
                processedLangs.Set(lang);
                Wi.Lemmas.TVector<TLemmaForms>::insert(Wi.Lemmas.end(), realNodes->begin(), realNodes->end());
            } else {
                TVector<TLemmaForms> generated;
                GenerateRealNodes(lf, generated);
                Wi.Lemmas.TVector<TLemmaForms>::insert(Wi.Lemmas.end(), generated.begin(), generated.end());
            }
            for (size_t j = base; j != Wi.Lemmas.size(); ++j) {
                Wi.Lemmas[j].Best &= bool(Wi.Lemmas[i - 1].IsExactLemma());
                Wi.Lemmas[j].ExactLemma = Wi.Lemmas[j].Best ? NLemmer::Lll : NLemmer::LllNo;
                Wi.Lemmas[j].FormType = Wi.Lemmas[i - 1].GetFormType();
                Wi.Lemmas[j].StopWord = Wi.Lemmas[i - 1].IsStopWord();
            }
        }
        Wi.Lemmas.erase(i - 1);
    }
}

TLangMask TWordInstanceUpdate::FilterLemmas(TLangMask lang) {
    Wi.LangMask &= ~lang;
    size_t j = Wi.Lemmas.size();
    while (j--) {
        if (lang.SafeTest(Wi.Lemmas[j].Language))
            Wi.ReduntLemma(j);
    }
    Y_ASSERT(!Wi.Lemmas.empty());
    return Wi.LangMask;
}
