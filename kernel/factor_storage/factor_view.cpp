#include "factor_view.h"
#include "factor_storage.h"

#include <util/generic/algorithm.h>

#include <library/cpp/sse/sse.h>

#if defined(__AVX__)
#include <util/system/cpu_id.h>
#include <immintrin.h>
#endif

namespace NFactorViewPrivate {
    void FloatClear(float *dst, float *end) {
    //#if defined(__AVX__)
    #if 0
        if (end > dst + 8 && NX86::CachedHaveAVX()) {
            __m256 zero = _mm256_setzero_ps();

            float* aligned = (float*) (size_t(dst) & ~size_t(31)) + 8;

            _mm256_storeu_ps(dst, zero);

            while (aligned + 32 < end) {
                _mm256_store_ps(aligned + 0, zero);
                _mm256_store_ps(aligned + 8, zero);
                _mm256_store_ps(aligned + 16, zero);
                _mm256_store_ps(aligned + 24, zero);
                aligned += 32;
            }

            while (aligned + 8 < end) {
                _mm256_store_ps(aligned, zero);
                aligned += 8;
            }

            _mm256_storeu_ps(end - 8, zero);

            _mm256_zeroupper();
            return;
        }
    #endif
    #ifdef ARCADIA_SSE
        if (end > dst + 4) {
            __m128 zero = _mm_setzero_ps();

            float *aligned = (float *)(size_t(dst) & ~size_t(0xf)) + 4;

            _mm_storeu_ps(dst, zero);

            while (aligned + 16 < end) {
                _mm_store_ps(aligned + 0, zero);
                _mm_store_ps(aligned + 4, zero);
                _mm_store_ps(aligned + 8, zero);
                _mm_store_ps(aligned + 12, zero);
                aligned += 16;
            }

            while (aligned + 4 < end) {
                _mm_store_ps(aligned + 0, zero);
                aligned += 4;
            }

            *(end - 4) = 0.0f;
            *(end - 3) = 0.0f;
            *(end - 2) = 0.0f;
            *(end - 1) = 0.0f;

            return;
        }
    #endif
        memset(dst, 0, (end - dst) * sizeof(float));
    }

    template <class ViewType, class SrcViewType>
    ViewType CreateAnySubView(const SrcViewType& fromView, EFactorSlice toSlice) {
        if (fromView.GetSlice() == toSlice) {
            return fromView;
        }
        Y_ASSERT(fromView.GetDomain());
        const TFactorDomain& domain = *fromView.GetDomain();
        const TSliceOffsets& fromOffsets = fromView.GetOffsets();
        const TSliceOffsets& toOffsets = domain[toSlice];
        Y_ASSERT(fromOffsets.Contains(toOffsets));

        if (Y_LIKELY(fromOffsets.Contains(toOffsets))) {
            return ViewType(toSlice, domain, ~fromView + fromOffsets.GetRelativeIndex(toOffsets.Begin));
        } else {
            return ViewType(0);
        }
    }
}

TConstFactorView::TConstFactorView(const TBasicFactorStorage& storage)
    : TConstFactorView(EFactorSlice::ALL, TSliceOffsets(0, storage.Size()), storage.Ptr(0))
{
}
TConstFactorView::TConstFactorView(const TFactorStorage& storage)
    : TConstFactorView(EFactorSlice::ALL, storage.GetDomain(), storage.Ptr(0))
{
}

TConstFactorView TConstFactorView::operator[] (EFactorSlice slice) const {
    return NFactorViewPrivate::CreateAnySubView<TConstFactorView>(*this, slice);
}

TFactorView::TFactorView(TBasicFactorStorage& storage)
    : TFactorView(EFactorSlice::ALL, TSliceOffsets(0, storage.Size()), storage.Ptr(0))
{
}
TFactorView::TFactorView(TFactorStorage& storage)
    : TFactorView(EFactorSlice::ALL, storage.GetDomain(), storage.Ptr(0))
{
}

TFactorView TFactorView::operator[] (EFactorSlice slice) const {
    return NFactorViewPrivate::CreateAnySubView<TFactorView>(*this, slice);
}

void TFactorView::FillCanonicalValues() {
    if (NFactorSlices::GetSlicesInfo()->IsHierarchical(GetSlice())) {
        for (NFactorSlices::TLeafIterator iter(GetSlice()); iter.Valid(); iter.Next()) {
            (*this)[*iter].FillCanonicalValues();
        }
    } else {
        Y_ASSERT(Info);
        if (Y_UNLIKELY(!Info)) {
            return;
        }

        const float* canonicalValues = Info->GetFactorCanonicalValues();

        if (Y_UNLIKELY(!canonicalValues)) {
            NFactorViewPrivate::FloatClear(begin(), end());
        } else {
            const auto valuesSize = Min(Info->GetFactorCount(), Size());
            Copy(canonicalValues, canonicalValues + valuesSize, begin());

            if (valuesSize < Size()) {
                NFactorViewPrivate::FloatClear(begin() + valuesSize, end());
            }
        }
    }
}
