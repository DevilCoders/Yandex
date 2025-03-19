#pragma once

#include "factor_view_base.h"

class TConstFactorView;
class TFactorView;
class TBasicFactorStorage;
class TFactorStorage;

class TConstFactorView : public TFactorViewBase<const float> {
    using TBase = TFactorViewBase<const float>;
public:
    explicit TConstFactorView(int)
    {
    }
    TConstFactorView(EFactorSlice slice, const TSliceOffsets& offsets, const float* factors)
        : TBase(slice, offsets, factors)
    {
    }
    TConstFactorView(EFactorSlice slice, const TFactorDomain& domain, const float* factors)
        : TBase(slice, domain, factors)
    {
    }
    TConstFactorView(const TFactorView& fv);
    TConstFactorView(const TBasicFactorStorage& storage);
    TConstFactorView(const TFactorStorage& storage);
    ~TConstFactorView() {
    }

    float operator[](size_t i) const {
        Y_ASSERT(i < Size());
        return *Ptr(i);
    }
    float operator[](const TFullFactorIndex& fullIndex) const {
        const float* ptr = Ptr(fullIndex);
        Y_ASSERT(ptr);
        return *ptr;
    }
    const float *GetConstFactors() const {
        return Ptr(0);
    }

    TConstFactorView operator[] (EFactorSlice slice) const;
};

class TFactorView : public TFactorViewBase<float> {
    using TBase = TFactorViewBase<float>;
public:
    explicit TFactorView(int)
    {
    }
    TFactorView(EFactorSlice slice, const TSliceOffsets& offsets, float* factors)
        : TFactorViewBase(slice, offsets, factors)
    {
    }
    TFactorView(EFactorSlice slice, const TFactorDomain& domain, float* factors)
        : TFactorViewBase(slice, domain, factors)
    {
    }
    TFactorView(TBasicFactorStorage& storage);
    TFactorView(TFactorStorage& storage);
    ~TFactorView() {
    }

    float& operator[](size_t i) const {
        Y_ASSERT(i < Size());
        return *Ptr(i);
    }
    float& operator[](const TFullFactorIndex& fullIndex) const {
        float* ptr = Ptr(fullIndex);
        Y_ASSERT(ptr);
        return *ptr;
    }
    float* GetRawFactors() const {
        return Ptr(0);
    }
    const float* GetConstFactors() const {
        return Ptr(0);
    }

    using TBase::Clear;
    using TBase::ClearFirstNFactors;

    void FillCanonicalValues();

    TFactorView operator[] (EFactorSlice slice) const;

    explicit operator TConstFactorView () const {
        return TConstFactorView(*this);
    }
};

using TMultiFactorView = TMultiFactorViewBase<TFactorView>;
using TMultiConstFactorView = TMultiFactorViewBase<TConstFactorView>;

template <ESliceRole Role>
using TFactorViewForRole = TFactorViewForBase<TFactorView, ESliceRole, Role>;
template <ESliceRole Role>
using TConstFactorViewForRole = TFactorViewForBase<TConstFactorView, ESliceRole, Role>;

template <EFactorSlice Slice>
using TFactorViewForSlice = TFactorViewForBase<TFactorView, EFactorSlice, Slice>;
template <EFactorSlice Slice>
using TConstFactorViewForSlice = TFactorViewForBase<TConstFactorView, EFactorSlice, Slice>;

class TFactorBuf
    : public TFactorView
{
public:
    TFactorBuf(ui32 count, float* rawFactors)
        : TFactorView(EFactorSlice::ALL, TSliceOffsets(0, count), rawFactors)
    {
        Y_ASSERT(count == 0 || !!rawFactors);
    }
};

inline TConstFactorView::TConstFactorView(const TFactorView& fv)
    : TBase(fv)
{
}
