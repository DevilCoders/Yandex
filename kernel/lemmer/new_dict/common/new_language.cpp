#include "new_language.h"

#include <kernel/search_types/search_types.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/morphfixlist.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <kernel/lemmer/new_engine/new_generator.h>
#include <kernel/lemmer/versions/versions.h>
#include <util/charset/wide.h>
#include <util/generic/yexception.h>

static bool HasGram(const TYandexLemma& lemma, EGrammar gram) {
    return NTGrammarProcessing::HasGram(lemma.GetStemGram(), gram) ||
           NTGrammarProcessing::HasGram(lemma.GetFlexGram(), lemma.FlexGramNum(), gram);
}

size_t TNewLanguage::RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
    size_t resultsNum = 0;
    if (opt.UseFixList && opt.Accept.Test(NLemmer::AccDictionary)) {
        const TLanguage* fix = NLemmer::TMorphFixList::SafeGetLanguage(NLemmer::GetMorphFixList(), Id);
        if (fix) {
            size_t base = out.size();
            resultsNum += fix->RecognizeInternal(word, length, out, opt);
            bool extend = false;
            for (size_t i = 0; i < resultsNum; ++i) {
                extend = extend || HasGram(out[base + i], gReserved);
            }
            if (resultsNum && !extend)
                return resultsNum;
        }
    }

    if (!GetLemmer())
        return 0;
    {
        size_t base = out.size();
        resultsNum += GetLemmer()->Analyze(TWtringBuf(word, length), out, opt);
        for (size_t i = base; i < out.size(); ++i) {
            NLemmerAux::TYandexLemmaSetter(out[i]).SetLanguageVersion(Version);
        }
    }
    //если нужны разборы из старой морфологии, и старые разборы для данного слова не покрываются новыми, то добавляем все старые разборы, с флагом Deprecated = true
    if (CheckOlderVersionsInRecognize(word, length, out, opt)) {
        const TLanguage* deprecatedLang = NLemmer::GetPreviousVersionOfLanguage(Id, GetVersion());
        if (deprecatedLang) {
            if (NLanguageVersioning::IsWordHashed(word, Id)) {
                return resultsNum;
            }
            NLemmer::TRecognizeOpt newOpt = opt;
            newOpt.UseFixList = false; //т.к. fixlist один на язык (и не зависит от версии словаря), и мы в него уже посмотрели, то при вызове старой морфологии еще раз смотреть в него не нужно
            newOpt.AllowDeprecated = false;
            size_t base = out.size();
            resultsNum += deprecatedLang->RecognizeInternal(word, length, out, newOpt);
            for (size_t i = base; i < out.size(); ++i) {
                NLemmerAux::TYandexLemmaSetter(out[i]).SetLanguageVersion(deprecatedLang->GetVersion());
            }
        }
    }
    return resultsNum;
}

TAutoPtr<NLemmer::TFormGenerator> TNewLanguage::Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter) const {
    Y_ASSERT(lemma.GetLanguage() == Id);
    Y_ASSERT(lemma.GetLanguageVersion() != -1);
    //если лемма сгенерирована старой морфологией - вызываем ее генератор
    if (lemma.GetLanguageVersion() != Version && lemma.GetLanguageVersion() != -1) {
        const TLanguage* lang = NLemmer::GetLanguageByVersion(Id, lemma.GetLanguageVersion());
        Y_ASSERT(lang);
        return lang->Generator(lemma, filter);
    }
    if (lemma.GetQuality() & TYandexLemma::QFix) {
        const TLanguage* lang = NLemmer::TMorphFixList::SafeGetLanguage(NLemmer::GetMorphFixList(), Id);
        if (lang) {
            return lang->Generator(lemma, filter);
        }
        return new TNewFormGenerator(lemma, nullptr, filter);
    }
    if (lemma.GetQuality() & TYandexLemma::QFoundling) {
        return new TNewFormGenerator(lemma, nullptr, filter);
    }
    return new TNewFormGenerator(lemma, GetLemmer(), filter);
}

TAutoPtr<NLemmer::TGrammarFiltr> TNewLanguage::GetGrammFiltr(const char* gram) const {
    TString f[] = {gram, ""};
    return new NLemmer::NDetail::TClueGrammarFiltr(f);
}

bool TNewLanguage::Spellcheck(const wchar16* word, size_t len) const {
    if (!GetLemmer())
        return false;
    Y_ASSERT(word);
    if (len >= MAXWORD_BUF)
        return false;

    wchar16* wordLo = (wchar16*)alloca(sizeof(wchar16) * len);
    AlphaRules().ToLower(+word, len, wordLo, len);

    NLemmer::TRecognizeOpt opt(NLemmer::AccDictionary);
    TWLemmaArray out;
    if (GetLemmer()->Analyze(TWtringBuf(wordLo, len), out, opt) != 0) {
        return true;
    }

    auto previous = NLemmer::GetPreviousVersionOfLanguage(Id, Version);
    if (previous && previous->Spellcheck(word, len)) {
        return true;
    }
    return false;
}
