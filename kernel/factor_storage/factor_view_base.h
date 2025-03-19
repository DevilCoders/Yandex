#pragma once

#include <kernel/factor_slices/factor_domain.h>

#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

namespace NFactorViewPrivate {
    void FloatClear(float *dst, float *end);
}

using NFactorSlices::EFactorSlice;
using NFactorSlices::EFactorUniverse;
using NFactorSlices::ESliceRole;
using NFactorSlices::TSliceOffsets;
using NFactorSlices::TFactorBorders;
using NFactorSlices::TSlicesMetaInfo;
using NFactorSlices::TFactorDomain;
using NFactorSlices::TFullFactorIndex;

template <class FloatType>
class TFactorViewBase {
private:
    FloatType* Factors = nullptr;
    EFactorSlice Slice = EFactorSlice::ALL;
    TSliceOffsets Offsets;

protected:
    const TFactorDomain* Domain = nullptr;
    const IFactorsInfo* Info = nullptr;

public:
    using TFloat = FloatType;

    TFactorViewBase() = default;
    TFactorViewBase(EFactorSlice slice, const TSliceOffsets& offsets, FloatType* factors);
    TFactorViewBase(EFactorSlice slice, const TFactorDomain& domain, FloatType* factors);

    template <class FloatTypeY>
    TFactorViewBase(const TFactorViewBase<FloatTypeY>& other)
        : Factors(static_cast<FloatType*>(~other))
        , Slice(other.GetSlice())
        , Offsets(other.GetOffsets())
        , Domain(other.GetDomain())
        , Info(other.GetDomain() && other.GetDomain()->IsNormal() ?
            other.GetDomain()->GetSliceFactorsInfo(other.GetSlice()) : nullptr)
    {
    }

    EFactorSlice GetSlice() const {
        return Slice;
    }
    const TSliceOffsets& GetOffsets() const {
        return Offsets;
    }
    const TFactorDomain* GetDomain() const {
        return Domain;
    }
    TOneFactorInfo GetFactorInfo(size_t index) const;
    TString GetPrintableFactorName(size_t index) const;
    void SetDomain(const TFactorDomain& domain);


    //All this members must die! {
    bool GetFactorIndex(const char* name, size_t* index, const IFactorsInfo& factorsInfo) const;
    const char* GetFactorName(size_t i, const IFactorsInfo& factorsInfo) const;
    bool IsBinary(size_t i, const IFactorsInfo& factorsInfo) const;
    bool IsRemovedFactor(size_t i, const IFactorsInfo& factorsInfo) const;
    //}
    void FilterFactorIdsForOtherSlice(TSet<ui32>& ids, EFactorSlice other) const;
    void MapFactorIdsFromOtherSlice(TSet<ui32>& ids, EFactorSlice other) const;
    TString CompressZlib() const;

    size_t Size() const {
        return Offsets.Size();
    }
    size_t size() const {return Size();}
    size_t operator+() const {
        return Offsets.Size();
    }
    FloatType* Ptr(size_t index) const {
        Y_ASSERT(index <= Offsets.Size());
        Y_ASSERT(Factors);
        return Factors + index;
    }
    bool Has(size_t offsetIndex) const {
        return offsetIndex < Offsets.Size();
    }
    bool Has(const TFullFactorIndex& fullIndex) const {
        if ((fullIndex.Slice == Slice
                && Offsets.ContainsRelative(fullIndex.Index))
            || (fullIndex.Slice == EFactorSlice::ALL
                && Offsets.Contains(fullIndex.Index)))
        {
            return true;
        } else if (Domain) {
            return Domain->HasIndex(fullIndex)
                && (*Domain)[Slice].Contains(Domain->GetIndex(fullIndex));
        }
        return false;
    }
    FloatType* Ptr(const TFullFactorIndex& fullIndex) const {
        Y_ASSERT(Has(fullIndex));
        Y_ASSERT(Factors);
        if (fullIndex.Slice == Slice) {
            return Factors + fullIndex.Index;
        } else if (fullIndex.Slice == EFactorSlice::ALL) {
            return Factors + Offsets.GetRelativeIndex(fullIndex.Index);
        } else if (Domain) {
            return Factors + Domain->GetRelativeIndex(Slice, fullIndex);
        } else {
            Y_ASSERT(false);
            return nullptr;
        }
    }
    FloatType* SafePtr(const TFullFactorIndex& fullIndex) const {
        if(!Has(fullIndex)) {
            return nullptr;
        }
        return Ptr(fullIndex);
    }
    FloatType* begin() const {
        return Factors;
    }
    FloatType* end() const {
        return Factors + Offsets.Size();
    }
    FloatType* operator~() const {
        return Factors;
    }
    explicit operator bool() const {
        return !!Factors;
    }

protected:
    void Clear() const;
    void ClearFirstNFactors(size_t n) const;
};

template <class ViewType>
class TMultiFactorViewBase {
public:
    using TView = ViewType;
    using TFloat = typename TView::TFloat;

    class TViewIterator;

public:
    template <typename Iter>
    TMultiFactorViewBase(Iter begin, Iter end,
        const TFactorDomain& domain,
        TFloat* factors)
        : Domain(&domain)
        , Factors(factors)
    {
        for (auto sliceIter : xrange(begin, end)) {
            for (NFactorSlices::TLeafIterator leafIter(*sliceIter); leafIter.Valid(); leafIter.Next()) {
                if (domain[*leafIter].Size() > 0) {
                    Slices.insert(*leafIter);
                }
            }
        }
    }

    TView operator[] (EFactorSlice slice) const;

    TViewIterator begin() const;
    TViewIterator end() const;

    size_t NumSlices() const;

private:
    TSet<EFactorSlice> Slices;
    const TFactorDomain* Domain = nullptr;
    TFloat* Factors = nullptr;
};

template <typename ViewType,
    typename SliceOrRoleType,
    SliceOrRoleType SliceOrRole>
class TFactorViewForBase
    : public ViewType
{
public:
    using TView = ViewType;

public:
    TFactorViewForBase() = default;

    TFactorViewForBase(const TFactorViewForBase&) = default;
    TFactorViewForBase& operator = (const TFactorViewForBase&) = default;

    TFactorViewForBase(const TView& view);
    TFactorViewForBase& operator = (const TView& view);

private:
    void AssignOrLeaveUnchanged(const TView&);
};

//
// TFactorViewBase
//

template <class FloatType>
inline TFactorViewBase<FloatType>::TFactorViewBase(NFactorSlices::EFactorSlice slice,
    const NFactorSlices::TSliceOffsets& offsets, FloatType* factors)
    : Factors(factors)
    , Slice(slice)
    , Offsets(offsets)
{
    Y_ASSERT(Offsets.Size() == 0 || !!Factors);
}

template <class FloatType>
inline TFactorViewBase<FloatType>::TFactorViewBase(NFactorSlices::EFactorSlice slice,
    const NFactorSlices::TFactorDomain& domain, FloatType* factors)
    : Factors(factors)
    , Slice(slice)
    , Domain(&domain)
    , Info(domain.IsNormal() ? domain.GetSliceFactorsInfo(slice) : nullptr)
{
    Offsets = (*Domain)[slice];
    Y_ASSERT(Offsets.Size() == 0 || !!Factors);
}

template <class FloatType>
inline void TFactorViewBase<FloatType>::SetDomain(const NFactorSlices::TFactorDomain& domain)
{
    Y_ASSERT(domain[Slice] == Offsets);
    Domain = &domain;
    Info = domain.GetSliceFactorsInfo(Slice);
}

template <class FloatType>
inline TOneFactorInfo TFactorViewBase<FloatType>::GetFactorInfo(size_t index) const
{
    Y_ASSERT(Offsets.ContainsRelative(index));
    if (Y_UNLIKELY(!Offsets.ContainsRelative(index))) {
        return TOneFactorInfo();
    }
    if (Info) {
        return TOneFactorInfo(index, Info);
    }
    if (Y_LIKELY(Domain)) {
        return Domain->GetFactorInfo(index, Slice);
    }
    return TOneFactorInfo();
}

template <class FloatType>
inline TString TFactorViewBase<FloatType>::GetPrintableFactorName(size_t index) const
{
    if (auto info = GetFactorInfo(index)) {
        return info.GetFactorName();
    } else {
        EFactorSlice slice = Slice;

        if (Domain) {
            EFactorSlice leaf = Domain->GetLeafByFactorIndex(index, Slice);
            if (EFactorSlice::COUNT != leaf) {
                slice = leaf;
                index = (*Domain)[leaf].GetRelativeIndex(index, Offsets);
            }
        }

        TStringStream out;
        Y_ASSERT(EFactorSlice::COUNT != slice);
        if (EFactorSlice::ALL == slice || EFactorSlice::COUNT == slice) {
            out << "unknown(" << index << ")";
        } else {
            out << slice << "(" << index << ")";
        }

        return out.Str();
    }
}

template <class FloatType>
inline bool TFactorViewBase<FloatType>::GetFactorIndex(const char* name, size_t* index,
    const IFactorsInfo& factorsInfo) const
{
    bool result = factorsInfo.GetFactorIndex(name, index);
    if (result) {
        *index = Offsets.GetRelativeIndex(*index);
    }
    return result;
}

template <class FloatType>
inline const char* TFactorViewBase<FloatType>::GetFactorName(size_t i,
    const IFactorsInfo& factorsInfo) const
{
    return factorsInfo.GetFactorName(Offsets.GetIndex(i));
}

template <class FloatType>
inline bool TFactorViewBase<FloatType>::IsBinary(size_t i, const IFactorsInfo& factorsInfo) const
{
    return factorsInfo.IsBinary(Offsets.GetIndex(i));
}

template <class FloatType>
inline bool TFactorViewBase<FloatType>::IsRemovedFactor(size_t i, const IFactorsInfo& factorsInfo) const
{
    return factorsInfo.IsRemovedFactor(Offsets.GetIndex(i));
}

template <class FloatType>
void TFactorViewBase<FloatType>::FilterFactorIdsForOtherSlice(TSet<ui32>& ids,
    EFactorSlice other) const
{
    if (!Domain)
        return;

    TSliceOffsets otherOffsets;
    otherOffsets = Domain->GetBorders()[other];
    TSet<ui32> idsNew;
    for (ui32 id: ids) {
        if (otherOffsets.Contains(id, Offsets)) {
            idsNew.insert(id);
        }
    }
    ids = idsNew;
}

template <class FloatType>
void TFactorViewBase<FloatType>::MapFactorIdsFromOtherSlice(TSet<ui32>& ids,
    EFactorSlice other) const
{
    if (!Domain)
        return;

    TSliceOffsets otherOffsets;
    otherOffsets = Domain->GetBorders()[other];
    TSet<ui32> idsNew;
    for (ui32 id: ids) {
        if (Offsets.Contains(id, otherOffsets)) {
            idsNew.insert(Offsets.GetRelativeIndex(id, otherOffsets));
        }
    }
    ids = idsNew;
}

template <class FloatType>
TString TFactorViewBase<FloatType>::CompressZlib() const
{
    TStringStream rfs;
    TZLibCompress compressor(&rfs, ZLib::ZLib);
    compressor.Write(Factors, sizeof(float) * Offsets.Size());
    compressor.Finish();
    return Base64Encode(rfs.Str());
}

template <class FloatType>
inline void TFactorViewBase<FloatType>::Clear() const
{
    NFactorViewPrivate::FloatClear(Factors, Factors + Offsets.Size());
}

template <class FloatType>
inline void TFactorViewBase<FloatType>::ClearFirstNFactors(size_t n) const
{
    Y_ASSERT(n < Offsets.Size());
    NFactorViewPrivate::FloatClear(Factors, Factors + n);
}

//
// TMultiFactorViewBase
//

template <class ViewType>
class TMultiFactorViewBase<ViewType>::TViewIterator {
public:
    using TParent = TMultiFactorViewBase<ViewType>;
    using TSlave = TSet<EFactorSlice>::const_iterator;
    using TView = typename TParent::TView;

public:
    TViewIterator(const TMultiFactorViewBase<ViewType>& parent,
        TSlave cur)
        : Parent(&parent)
        , Cur(cur)
    {}

    TView operator*() const {
        return (*Parent)[*Cur];
    }
    void operator++() {
        ++Cur;
    }
    bool operator==(const TViewIterator& other) const {
        Y_ASSERT(Parent == other.Parent);
        return Cur == other.Cur;
    }
    bool operator!=(const TViewIterator& other) const {
        Y_ASSERT(Parent == other.Parent);
        return Cur != other.Cur;
    }

private:
    const TParent* Parent = nullptr;
    TSlave Cur;
};

template <class ViewType>
inline typename TMultiFactorViewBase<ViewType>::TView TMultiFactorViewBase<ViewType>::operator[] (EFactorSlice slice) const
{
    Y_ASSERT(Factors);
    Y_ASSERT(Domain);
    return TView(slice, *Domain, Factors + (*Domain)[slice].Begin);
}

template <class ViewType>
inline typename TMultiFactorViewBase<ViewType>::TViewIterator TMultiFactorViewBase<ViewType>::begin() const
{
    return TViewIterator(*this, Slices.begin());
}

template <class ViewType>
inline typename TMultiFactorViewBase<ViewType>::TViewIterator TMultiFactorViewBase<ViewType>::end() const
{
    return TViewIterator(*this, Slices.end());
}

template <class ViewType>
inline size_t TMultiFactorViewBase<ViewType>::NumSlices() const
{
    return Slices.size();
}

//
// TFactorViewForBase
//

template <typename ViewType, typename SliceOrRoleType, SliceOrRoleType SliceOrRole>
TFactorViewForBase<ViewType, SliceOrRoleType, SliceOrRole>::TFactorViewForBase(const TView& view)
    : TView(0)
{
    AssignOrLeaveUnchanged(view);
}

template <typename ViewType, typename SliceOrRoleType, SliceOrRoleType SliceOrRole>
TFactorViewForBase<ViewType, SliceOrRoleType, SliceOrRole>&
TFactorViewForBase<ViewType, SliceOrRoleType, SliceOrRole>::operator = (const TView& view)
{
    TView::operator = (TView(0));
    AssignOrLeaveUnchanged(view);
    return *this;
}

template <typename ViewType, typename SliceOrRoleType, SliceOrRoleType SliceOrRole>
void TFactorViewForBase<ViewType, SliceOrRoleType, SliceOrRole>::AssignOrLeaveUnchanged(const TView& view)
{
    if (Y_UNLIKELY(!view.GetDomain())) {
        Y_ASSERT(false);
        return;
    }

    if (view.GetDomain()->GetSliceFor(SliceOrRole) == view.GetSlice()) {
        TView::operator = (view);
        return;
    }

    if (EFactorSlice::ALL == view.GetSlice()) {
        TView::operator = (view[view.GetDomain()->GetSliceFor(SliceOrRole)]);
    } else {
        Y_ASSERT(false);
    }
}
