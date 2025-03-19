#include "ngrams.h"
#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/core/language.h>

namespace {
    struct TEmptyAlphaMap: public NLemmer::NDetail::TAlphaMap {
        TEmptyAlphaMap()
            : TAlphaMap(nullptr)
        {
        }
    };

}

namespace NLemmer {
    namespace NDetail {
        static const TNGramData::TNGram ArrNGramEmpty[] = {TNGramData::TNGram()};
        static const size_t ArrNGramEmptySize = 0;
        const TNGramData TNGramData::Empty = {ArrNGramEmpty, ArrNGramEmptySize};

        const TAlphaMap& TAlphaMap::Empty() {
            return *HugeSingleton<TEmptyAlphaMap>();
        }

        TAlphaMap::TAlphaMap(const NLemmer::TAlphabetWordNormalizer* awn) {
            memset(Map, 0, sizeof(Map));
            if (!awn)
                return;
            size_t n = 0;
            Map[32] = ++n;
            for (wchar16 alpha = 65535; alpha > 0; --alpha) {
                TAlphabet::TCharClass c = awn->GetCharClass(alpha);
                if (c & (TAlphabet::CharAlphaNormal | TAlphabet::CharSign)) {
                    wchar16 la = awn->ToLower(alpha);
                    if (!Map[la]) {
                        Map[la] = ++n;
                        Y_ASSERT(n <= MaxMappedNum);
                    }
                }
            }
        }
    }
}
