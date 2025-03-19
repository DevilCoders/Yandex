#pragma once

#include "decompression.h"

#include <cmath>
#include <utility>

#include <library/cpp/sse/sse.h>

namespace NDssmApplier {
    namespace NPrivate {
        // DSSM compression is linear: byte i (0 <= i <= 255) denotes floating point value
        //   ai+b = a*i + b
        // Where
        //   step = (max - min) / 256.0
        //   b = min + 0.5 * step
        //   a = step
        //   ('min' and 'max' are specified in DSSM_MODEL_TO_BOUNDS)
        // Hence scalar product is the sum of 'Dimension' (~50) terms of the form
        //   (a * i + b) * (a * j + b)
        // And after division by a^2 we obtain:
        //   (i + b / a) * (j + b / a)
        // The distance is therefore proportional to (sums are taken over 'Dimension' byte pairs (i, j))
        //   (Sum i*j) + ((Sum i) + (Sum j)) * (b / a) + (b / a) ^ 2
        // The last term is constant and does not affect the distance comparison result
        // Calculation of the first two terms can be optimized using intrinsics.
        template <class Derived>
        class TFastDistanceBase {
        public:
            using TResult = float;

            using TLess = TGreater<TResult>;

            explicit TFastDistanceBase(EDssmModelType dssmModelType)
            {
                const std::pair<float, float> bounds = GetBounds(dssmModelType);
                const float step = (bounds.second - bounds.first) / std::tuple_size<TDecompression>::value;
                const float a = step;
                const float b = bounds.first + step * 0.5f;

                Coef = b / a;
            }

            TResult operator()(const TArrayRef<const ui8>& a, const TArrayRef<const ui8>& b) const {
                Y_ASSERT(a.size() == b.size());
                return operator()(a.data(), b.data(), a.size());
            }

            TResult operator()(const ui8* a, const ui8* b, const size_t dimension) const {
                i32 scalarProduct = 0;
                i32 sumComps = 0;

                static_cast<const Derived*>(this)->Calculate(a, b, dimension, scalarProduct, sumComps);

                return scalarProduct + sumComps * Coef;
            }

        private:
            float Coef = 0;
        };

        class TFastDistancePlain
           : public TFastDistanceBase<TFastDistancePlain> {
        public:
            using TFastDistanceBase<TFastDistancePlain>::TFastDistanceBase;

            void Calculate(const ui8* a, const ui8* b, const size_t dimension, i32& scalarProduct, i32& sumComps) const {
                for (size_t i = 0; i < dimension; ++i) {
                    scalarProduct += a[i] * b[i];
                    sumComps += a[i] + b[i];
                }
            }
        };

#ifdef ARCADIA_SSE
        class TFastDistanceSse2
           : public TFastDistanceBase<TFastDistanceSse2> {
        public:
            using TFastDistanceBase<TFastDistanceSse2>::TFastDistanceBase;

            void Calculate(const ui8* a, const ui8* b, const size_t dimension, i32& scalarProductRes, i32& sumCompsRes) const {
                // 4 x ui32, because scalar products are < 256^2 * Dimension
                __m128i scalarProduct = _mm_setzero_si128();

                // 8 x ui16, because component sums are < 256 * Dimension
                __m128i sumComps = _mm_setzero_si128();

                const __m128i zero = _mm_setzero_si128();

                const ui8* pa = a;
                const ui8* pb = b;

                for (size_t i = 0; i < dimension / 16; ++i) {
                    // Load 16 x ui8 from each vector
                    __m128i aVec = _mm_loadu_si128((const __m128i*)pa);
                    __m128i bVec = _mm_loadu_si128((const __m128i*)pb);

                    // Extend with zero bytes: one ui8-vector -> two u16-vectors
                    // Each of aLo, aHi, bLo, bHi holds 8 x ui16
                    __m128i aLo = _mm_unpacklo_epi8(aVec, zero);
                    __m128i bLo = _mm_unpacklo_epi8(bVec, zero);
                    __m128i aHi = _mm_unpackhi_epi8(aVec, zero);
                    __m128i bHi = _mm_unpackhi_epi8(bVec, zero);

                    sumComps = _mm_add_epi16(sumComps, aLo);
                    sumComps = _mm_add_epi16(sumComps, bLo);
                    sumComps = _mm_add_epi16(sumComps, aHi);
                    sumComps = _mm_add_epi16(sumComps, bHi);

                    scalarProduct = _mm_add_epi32(scalarProduct, _mm_madd_epi16(aLo, bLo));
                    scalarProduct = _mm_add_epi32(scalarProduct, _mm_madd_epi16(aHi, bHi));

                    pa += 16;
                    pb += 16;
                }

                // Convolution of 4 x ui32 sums into one ui32 sum
                scalarProduct = _mm_add_epi32(scalarProduct, _mm_srli_si128(scalarProduct, 8));
                scalarProduct = _mm_add_epi32(scalarProduct, _mm_srli_si128(scalarProduct, 4));
                scalarProductRes = _mm_cvtsi128_si32(scalarProduct);

                // Convolution of 8 x ui16 sums into one ui16 sum
                sumComps = _mm_add_epi16(sumComps, _mm_srli_si128(sumComps, 2));
                sumComps = _mm_add_epi16(sumComps, _mm_srli_si128(sumComps, 4));
                sumComps = _mm_add_epi16(sumComps, _mm_srli_si128(sumComps, 8));
                sumCompsRes = static_cast<i32>(_mm_cvtsi128_si32(sumComps) & 0xFFFF);

                // Process the rest in a non-vectorized manner
                for (size_t i = 0; i < dimension % 16; ++i) {
                    sumCompsRes += pa[0];
                    sumCompsRes += pb[0];
                    scalarProductRes += pa[0] * static_cast<ui16>(pb[0]);
                    pa += 1;
                    pb += 1;
                }
            }
        };
#endif

    }

#ifdef ARCADIA_SSE
    using TFastDistance = NPrivate::TFastDistanceSse2;
#else
    using TFastDistance = NPrivate::TFastDistancePlain;
#endif

}
