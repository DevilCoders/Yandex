#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/utility.h>
#include <util/generic/singleton.h>
#include <library/cpp/charset/codepage.h>
#include <util/charset/wide.h>

#include "defaultlanguage.h"
#include "language.h"
#include "lemmeraux.h"
#include "rubbishdump.h"

TLanguage::~TLanguage() {
}

TLanguage::TLanguage(ELanguage id, bool dontRegister, int version)
    : Id(id)
    , Version(version)
{
    if (!dontRegister)
        RegisterLanguage();
    if (!NLemmer::GetAlphaRules(Id))
        ythrow yexception() << "no alpha rules for language " << static_cast<size_t>(id) << Endl;
}

bool TLanguage::CanBreak(const wchar16*, size_t, size_t, size_t, size_t, bool) const {
    return true;
}

bool TLanguage::Spellcheck(const wchar16*, size_t) const {
    return false;
}

TAutoPtr<NLemmer::TFormGenerator> TLanguage::Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter) const {
    //ythrow yexception() << "no default generation";
    Y_ASSERT(lemma.GetLanguage() == Id || NLemmer::GetLanguageByIdAnyway(lemma.GetLanguage())->Id == Id);
    return new NLemmer::TFormGenerator(lemma, filter);
    //return new NLemmer::TBykFormGenerator(lemma, nullptr, filter);
}

TAutoPtr<NLemmer::TGrammarFiltr> TLanguage::GetGrammFiltr(const char*) const {
    return new NLemmer::NDetail::TDummyGrammarFiltr;
}

// TODO: move out
size_t NLemmer::Generate(const TYandexLemma& lemma, TWordformArray& out, const char* needed_grammar) {
    const TLanguage* lang = GetLanguageByIdAnyway(lemma.GetLanguage());
    TAutoPtr<TFormGenerator> gen = lemma.Generator(lang->GetGrammFiltr(needed_grammar).Get());
    TYandexWordform form;
    while (gen->GenerateNext(form))
        out.push_back(form);

    return out.size();
}

TVector<TVector<TUtf16String>> NLemmer::GetWordForms(const TUtf16String& word, ELanguage langId) {
    auto lang = GetLanguageById(langId);
    if (!lang) {
        return TVector<TVector<TUtf16String>>(1, TVector<TUtf16String>(1, word));
    }
    TWLemmaArray lemmas;
    lang->Recognize(word.c_str(), word.size(), lemmas, NLemmer::TRecognizeOpt());
    TVector<TVector<TUtf16String>> result(lemmas.size());
    for (size_t i = 0; i < lemmas.size(); ++i) {
        TWordformArray forms;
        lang->Generate(lemmas[i], forms, nullptr);
        for (const auto& form : forms) {
            result[i].push_back(form.GetText());
        }
    }
    return result;
}

size_t TLanguage::Generate(const TYandexLemma& lemma, TWordformArray& out, const char* needed_grammar) const {
    TAutoPtr<NLemmer::TFormGenerator> gen = Generator(lemma, GetGrammFiltr(needed_grammar).Get());
    TYandexWordform form;
    while (gen->GenerateNext(form))
        out.push_back(form);

    return out.size();
}

size_t TLanguage::Recognize(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
    out.clear();
    return RecognizeAdd(word, length, out, opt);
}

size_t TLanguage::RecognizeAdd(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
    TConvertResults cr(word, length);
    if (!opt.SkipNormalization) {
        cr.PreConvertRet = AlphaRules().PreConvert(word, length, cr.PreConvertBuf, TConvertResults::PreConvertBufSize);
        cr.PreConverted = cr.PreConvertBuf;
        if (!cr.PreConvertRet.Valid && !opt.SkipValidation)
            return 0;

        cr.ConvertRet = Normalize(
            cr.PreConvertBuf,
            cr.PreConvertRet.Length,
            cr.ConvertBuf,
            TConvertResults::ConvertBufSize,
            NLemmer::TAlphabetWordNormalizer::ModeConvertNormalized(opt.SkipAdvancedNormaliation));

        cr.Converted = cr.ConvertBuf;
        if (!cr.ConvertRet.Valid && !opt.SkipValidation)
            return 0;
    }

    size_t base = out.size();
    size_t num = RecognizeInternal(cr.Converted, cr.ConvertRet.Length, out, opt);
    if (!num && opt.Accept.Test(NLemmer::AccFoundling)) {
        if (!opt.AllowEmptyLemma && cr.ConvertRet.Length == 0) {
            //no empty lemmas (in case tokenizer created token that becomes empty after normaliztion - for example, surrogate pair punctuation)
            return 0;
        }
        num = RecognizeInternalFoundling(cr.Converted, cr.ConvertRet.Length, out, opt.MaxLemmasToReturn);
    }

    for (size_t i = 0; i != num; ++i) {
        NLemmerAux::TYandexLemmaSetter set(out[base + i]);
        set.SetLanguage(Id);
        set.SetInitialForm(word, length);
        set.SetNormalizedForm(cr.PreConverted, cr.PreConvertRet.Length);
        set.SetConvertedFormLength(cr.ConvertRet.Length);
    }
    num = PostRecognize(out, base, base + num, cr, opt);

    return num;
}

size_t TLanguage::RecognizeInternalFoundling(const wchar16* word, size_t length, TWLemmaArray& out, size_t maxw) const {
    if (maxw == 0)
        return 0;
    out.push_back(TYandexLemma());
    NLemmerAux::TYandexLemmaSetter set(out.back());
    set.SetLemma(word, length, 0, 0, 0, "");
    set.SetQuality(TYandexLemma::QFoundling);
    return 1;
}

size_t TLanguage::RecognizeInternal(const wchar16*, size_t, TWLemmaArray&, const NLemmer::TRecognizeOpt&) const {
    return 0;
}

bool TLanguage::CheckOlderVersionsInRecognize(const wchar16*, size_t, const TWLemmaArray&, const NLemmer::TRecognizeOpt& opt) const {
    return opt.AllowDeprecated;
}

size_t TLanguage::PostRecognize(TWLemmaArray&, size_t begin, size_t end, const TConvertResults&, const NLemmer::TRecognizeOpt&) const {
    return end - begin;
}

NLemmer::TLllLevel TLanguage::LooksLikeLemma(const TYandexLemma& lemma) const {
    return NLanguageRubbishDump::DefaultLooksLikeLemma(lemma);
}

TAutoPtr<NLemmer::TGrammarFiltr> TLanguage::LooksLikeLemmaExt(const TYandexLemma& lemma) const {
    if (!LooksLikeLemma(lemma))
        return nullptr;
    TAutoPtr<NLemmer::TGrammarFiltr> p = GetGrammFiltr("");
    p->SetLemma(lemma);
    return p;
}

bool TLanguage::BelongsToLanguage(const TYandexLemma& lemma) const {
    static const ui32 QNotBelongs = TYandexLemma::QFromEnglish | TYandexLemma::QToEnglish | TYandexLemma::QUntranslit;
    return lemma.GetLanguage() == Id && !(lemma.GetQuality() & QNotBelongs);
}

TAutoPtr<TUntransliter> TLanguage::GetUntransliter(const TUtf16String&) const {
    return nullptr;
}

TAutoPtr<TUntransliter> TLanguage::GetTransliter(const TUtf16String&) const {
    return nullptr;
}

TAutoPtr<IDetransliterator> TLanguage::GetDetransliterator() const {
    return nullptr;
}

const TTranslationDict* TLanguage::GetUrlTransliteratorDict() const {
    return nullptr;
}

size_t TLanguage::FromTranslit(const wchar16* word, size_t length, TWLemmaArray& out, size_t max, const NLemmer::TTranslitOpt* opt) const {
    out.clear();
    return FromTranslitAdd(word, length, out, max, opt);
}
