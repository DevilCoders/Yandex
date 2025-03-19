#include <util/charset/wide.h>
#include <util/generic/map.h>
#include <util/generic/utility.h>

#include "language.h"
#include "lemmeraux.h"

static const size_t MaxTranslitNum = 10;

using NLemmer::TAlphabetWordNormalizer;
using NLemmer::TTranslitOpt;

static void addLemmas(TMap<TYandexLemma, size_t>& to_ret, TWLemmaArray& lemmas, const NLemmer::TLanguageQualityAccept& exactForms) {
    using namespace NLemmer;
    for (size_t i = 0; i < lemmas.size(); ++i) {
        if (exactForms.Test(AccDictionary) && (lemmas[i].GetQuality() & TYandexLemma::QAnyBastard) == TYandexLemma::QDictionary || exactForms.Test(AccSob) && (lemmas[i].GetQuality() & TYandexLemma::QAnyBastard) == TYandexLemma::QSob || exactForms.Test(AccBastard) && (lemmas[i].GetQuality() & TYandexLemma::QAnyBastard) == TYandexLemma::QBastard) {
            NLemmerAux::TYandexLemmaSetter(lemmas[i]).ResetLemmaToForm();
        }

        if (to_ret.find(lemmas[i]) != to_ret.end())
            continue;
        size_t sz = to_ret.size();
        to_ret[lemmas[i]] = sz;
    }
}

std::pair<size_t, TUtf16String> TLanguage::UntranslitIt(const TConvertResults& cr, size_t length, TWLemmaArray& out, size_t maxw, const NLemmer::TTranslitOpt* opt, bool tryPrecise) const {
    typedef TMap<TYandexLemma, size_t> TL2I;
    TL2I to_ret;
    std::pair<size_t, TUtf16String> ret(0, TUtf16String());
    NLemmer::TLanguageQualityAccept accept = (opt == nullptr) ? NLemmer::TLanguageQualityAccept(NLemmer::AccFoundling) : NLemmerAux::QualitySetByMaxLevel(NLemmer::AccBastard);
    NLemmer::TRecognizeOpt recOpt(MAX_LEMMS, accept, opt ? opt->NeedGramm : "", true);
    TWLemmaArray buffer;
    if (tryPrecise && GetDetransliterator()) {
        auto detranslit = GetDetransliterator()->Convert(TUtf16String(cr.PreConverted, length));
        ret.second = detranslit;
        Recognize(detranslit.c_str(), detranslit.length(), buffer, recOpt);
        addLemmas(to_ret, buffer, opt ? opt->ExactForms : NLemmer::TLanguageQualityAccept());
    } else {
        TAutoPtr<TUntransliter> un = GetUntransliter(TUtf16String(cr.Converted, length));
        if (!un.Get())
            return ret;

        TUntransliter::WordPart wp = un->GetNextAnswer();
        if (wp.Empty())
            return ret;

        ui32 worstQ = 0;
        {
            size_t n = (size_t)(wp.Quality() * TUntransliter::DefaultQualityRange + 0.5);
            worstQ = (n > ui32(-1)) ? ui32(-1) : ui32(n);
        }
        if ((size_t)wp.Quality() < TUntransliter::WorstQuality)
            ret.second = wp.GetWord();
        {
            size_t count = 0;
            recOpt.SkipNormalization = true;
            do {
                size_t n = Recognize(wp.GetWord().c_str(), wp.GetWord().length(), buffer, recOpt);
                if (n > 0 && !buffer[0].IsBastard() && recOpt.Accept.Test(NLemmer::AccDictionary) && (recOpt.Accept.Test(NLemmer::AccBastard) || recOpt.Accept.Test(NLemmer::AccSob))) {
                    recOpt.Accept.Reset(NLemmer::AccBastard);
                    recOpt.Accept.Reset(NLemmer::AccSob);
                    to_ret.clear();
                }
                if (to_ret.size() < maxw)
                    addLemmas(to_ret, buffer, opt ? opt->ExactForms : NLemmer::TLanguageQualityAccept());
                else if (!recOpt.Accept.Test(NLemmer::AccDictionary) || !recOpt.Accept.Test(NLemmer::AccBastard))
                    break;
            } while (!(wp = un->GetNextAnswer(worstQ)).Empty() && (++count < MaxTranslitNum));
        }
    }
    ret.first = 0;
    size_t base = out.size();
    out.resize(base + Min(maxw, to_ret.size()));
    for (TL2I::const_iterator i = to_ret.begin(); i != to_ret.end(); ++i) {
        if (i->second >= maxw)
            continue;
        if (i->second >= ret.first)
            ret.first = i->second + 1;
        out[base + i->second] = i->first;
    }
    return ret;
}

size_t TLanguage::FromTranslitAdd(const wchar16* word, size_t length, TWLemmaArray& out, size_t maxw, const NLemmer::TTranslitOpt* opt, bool tryPrecise) const {
    if (!GetDetransliterator() && !GetUntransliter(TUtf16String(word, length)).Get()) {
        return 0;
    }
    TConvertResults cr(word, length);
    const NLemmer::TAlphabetWordNormalizer* alphaRules = NLemmer::GetAlphaRulesUnsafe(Id, false);
    Y_ENSURE(alphaRules, "no translit rules for " << Id);
    cr.ConvertRet = alphaRules->Normalize(word, length, cr.ConvertBuf, TConvertResults::ConvertBufSize,
                                          TAlphabetWordNormalizer::ModePreConvert() | TAlphabetWordNormalizer::ModeConvertNormalized());
    cr.Converted = cr.ConvertBuf;
    if (!cr.ConvertRet.Valid)
        return 0;

    size_t base = out.size();
    std::pair<size_t, TUtf16String> rt = UntranslitIt(cr, length, out, maxw, opt, tryPrecise);

    if (!rt.first && rt.second && maxw) {
        rt.first = RecognizeInternalFoundling(rt.second.c_str(), rt.second.length(), out, 1);
        if (rt.first) {
            NLemmerAux::TYandexLemmaSetter set(out.back());
            set.SetQuality(TYandexLemma::QBastard);
            set.SetNormalizedForm(rt.second.c_str(), rt.second.length());
            set.SetLanguage(Id);
        }
    }

    for (size_t i = 0; i < rt.first; ++i) {
        NLemmerAux::TYandexLemmaSetter set(out[base + i]);
        set.SetInitialForm(word, length);
        set.SetQuality(out[base + i].GetQuality() | TYandexLemma::QUntranslit);
    }
    rt.first = PostRecognize(out, base, base + rt.first, cr, NLemmer::TRecognizeOpt());

    return rt.first;
}
