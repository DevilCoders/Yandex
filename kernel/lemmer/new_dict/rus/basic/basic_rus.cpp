#include "basic_rus.h"

#include <kernel/lemmer/core/lemmeraux.h>

size_t TBasicRusLanguage::PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults&, const NLemmer::TRecognizeOpt&) const {
    for (size_t i = begin; i < end; ++i) {
        NLemmerAux::TYandexLemmaSetter set(lemmas[i]);
        set.SetLanguage(LANG_RUS);
    }
    return end - begin;
}
