#include "tur.h"

#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <library/cpp/tokenizer/nlpparser.h>
#include <util/generic/string.h>

#include <utility>

bool TTurLanguage::CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool /*isTranslit*/) const {
    if (pos1 + len1 + 1 != pos2 || !len2 || !len1)
        return true;
    return TNlpParser::GetCharClass(word[pos1 + len1]) != TNlpParser::CC_APOSTROPHE;
}

template <typename T>
bool TTurLanguage::FirstPartIsAForm(T lemmasBegin, T lemmasEnd, const TWtringBuf& firstPart, const NLemmer::TRecognizeOpt& opt) const {
    using namespace NLemmer;
    using namespace NTGrammarProcessing;
    TWLemmaArray newLemmas;
    if (lemmasBegin != lemmasEnd && lemmasBegin->GetQuality() == TYandexLemma::QDictionary && !HasGram(lemmasBegin->GetAddGram(), gDistort)) {
        NLemmer::TRecognizeOpt opt2(opt);
        opt2.Accept = TLanguageQualityAccept(AccDictionary);
        size_t num = RecognizeInternal(firstPart.data(), firstPart.size(), newLemmas, opt2);
        for (T iter = lemmasBegin; iter != lemmasEnd; ++iter) {
            for (size_t i = 0; i != num; ++i) {
                if (iter->Similar(newLemmas[i]))
                    return true;
            }
        }
    }
    return false;
}

size_t TTurLanguage::PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults& cr, const NLemmer::TRecognizeOpt& opt) const {
    using namespace NLemmer;
    using namespace NLemmerAux;

    if (!opt.Accept.SafeTest(AccBastard))
        return end - begin;

    TWtringBuf preConverted(cr.PreConverted, cr.PreConvertRet.Length);
    size_t apos = preConverted.rfind(u'\'');
    if (apos == TWtringBuf::npos || apos >= cr.ConvertRet.Length)
        return end - begin;
    if (apos != preConverted.find(u'\'')) // exclude words with double apostrophes like "ku'ran'da"
        return end - begin;

    TWtringBuf firstPart(cr.Converted, apos);
    if (FirstPartIsAForm(lemmas.begin() + begin, lemmas.begin() + end, firstPart, opt))
        return end - begin;

    TWtringBuf converted(cr.Converted, cr.ConvertRet.Length);
    TUtf16String convertedWithApos = TUtf16String::Join(firstPart, u"\'", converted.substr(apos));
    TWLemmaArray newLemmas;
    size_t num = RecognizeInternal(convertedWithApos.data(), convertedWithApos.size(), newLemmas, opt);
    size_t base = lemmas.size();
    for (size_t i = 0; i != num; ++i) {
        if (newLemmas[i].GetTextLength() != apos)
            continue;
        TYandexLemmaSetter set(newLemmas[i]);
        set.SetLanguage(Id);
        set.SetInitialForm(cr.Word.data(), cr.Word.size());
        set.SetNormalizedForm(cr.PreConverted, cr.PreConvertRet.Length);
        set.SetConvertedFormLength(convertedWithApos.size());
        set.SetQuality(TYandexLemma::QDictionary | TYandexLemma::QOverrode);
        lemmas.insert(lemmas.begin(), std::move(newLemmas[i])); // yes, this is inefficient
    }
    return lemmas.size() - base + end - begin;
}
