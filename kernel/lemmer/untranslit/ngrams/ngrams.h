#pragma once

#include <stddef.h>
#include <util/system/types.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/system/yassert.h>

namespace NLemmer {
    class TAlphabetWordNormalizer;
}
namespace NLemmer {
    namespace NDetail {
        class TAlphaMap {
            static_assert(sizeof(wchar16) == 2, "expect sizeof(wchar16) == 2");
            static const size_t MapSize = 65536;
            static const size_t MaxMappedNum = 63;
            unsigned char Map[MapSize];

        public:
            TAlphaMap(const NLemmer::TAlphabetWordNormalizer* awn);

            unsigned char operator()(wchar16 alpha) const {
                return Map[alpha];
            }
            static const TAlphaMap& Empty();
        };

        struct TNGramData {
        public:
            typedef ui32 TNGramCode;
            struct TNGram {
                TNGramCode NGr;
                ui32 Weight;
            };

        public:
            static const unsigned int NgrSize = 5;
            static const ui32 BadChar = 0;
            static const TNGramData Empty;

        public:
            const TNGram* NGramsArr;
            size_t NGramArrSize;

        private:
            size_t LookForNGr(TNGramCode ngram) const {
                if (NGramArrSize < 2 || ngram < NGramsArr[1].NGr) //frequent case optimization
                    return 0;
                size_t i = 0; //std::binary_search медленнее
                size_t n = NGramArrSize;
                while (n > 0) {
                    size_t half = n >> 1;
                    size_t probe = i + half;
                    if (NGramsArr[probe].NGr < ngram) {
                        i = probe + 1;
                        n = n - half - 1;
                    } else {
                        n = half;
                    }
                }
                return i;
            }

        public:
            static TNGramCode GetNGrCode(const wchar16* ngram, size_t length, const TAlphaMap& aplhaMap) {
                Y_ASSERT(length <= NgrSize);
                TNGramCode r = 0;
                for (size_t i = 0; i < length; ++i) {
                    TNGramCode n = aplhaMap(ngram[i]);
                    if (!n)
                        return BadChar;
                    r += (n << i * 6);
                }
                return r;
            }

            ui32 LookForNGr(const wchar16* ngram, const TAlphaMap& aplhaMap) const {
                Y_ASSERT(NGramArrSize > 0);
                TNGramCode r = GetNGrCode(ngram, NgrSize, aplhaMap);
                ;
                size_t i = LookForNGr(r);
                if (i != NGramArrSize && NGramsArr[i].NGr == r)
                    return NGramsArr[i].Weight;

                r = GetNGrCode(ngram, NgrSize - 1, aplhaMap);
                ;
                i = LookForNGr(r);
                if (i != NGramArrSize && NGramsArr[i].NGr == r)
                    return NGramsArr[i].Weight;

                return NGramsArr->Weight;
            }

            bool IsEmpty() const {
                return !NGramArrSize;
            }
        };

        inline bool operator<(TNGramData::TNGram t1, TNGramData::TNGram t2) {
            return t1.NGr < t2.NGr;
        }
    }
}
