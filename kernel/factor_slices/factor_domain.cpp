#include "factor_domain.h"

namespace NFactorSlices {
    EFactorUniverse TFactorDomain::GlobalUniverse = EFactorUniverse::DEFAULT;

    void TFactorDomain::ForceSetGlobalUniverse(EFactorUniverse universe)
    {
        GlobalUniverse = universe;
    }

    void TFactorDomain::SetGlobalUniverse(EFactorUniverse universe)
    {
        Y_ASSERT(GlobalUniverse == universe || GlobalUniverse == NFactorSlices::EFactorUniverse::DEFAULT);
        ForceSetGlobalUniverse(universe);
    }

    EFactorUniverse TFactorDomain::GetGlobalUniverse()
    {
        return GlobalUniverse;
    }

    EFactorUniverse TFactorDomain::GetUniverse() const
    {
        return Universe.GetOrElse(GlobalUniverse);
    }

    EFactorSlice TFactorDomain::GetSliceFor(EFactorUniverse universe, ESliceRole role)
    {
        return GetSlicesInfo()->GetSliceFor(universe, role);
    }

    EFactorSlice TFactorDomain::GetSliceFor(ESliceRole role) const
    {
        return GetSlicesInfo()->GetSliceFor(GetUniverse(), role);
    }

    [[nodiscard]] TFactorDomain TFactorDomain::MakeDomainWithIncreasedSlice(NFactorSlices::EFactorSlice slice, size_t newFeatsNum) const {
        NFactorSlices::TSlicesMetaInfo meta;
        Y_ENSURE(NFactorSlices::NDetail::ReConstructMetaInfo(this->GetBorders(), meta));

        meta.SetNumFactors(slice, meta.GetNumFactors(slice) + newFeatsNum);

        NFactorSlices::EnableSlices(meta, slice);

        return NFactorSlices::TFactorDomain(NFactorSlices::TFactorBorders(meta));
    }

    TMaybe<TFullFactorIndex> TFactorDomain::GetL3ModelValueIndex() const {
        TMaybe<TFullFactorIndex> l3ModelValueIndex;
        EFactorSlice slice = GetSliceFor(ESliceRole::MAIN);
        const IFactorsInfo* factorsInfo = GetSliceFactorsInfo(slice);
        if (factorsInfo) {
            TMaybe<size_t> mxNetIndex = factorsInfo->GetL3ModelValueIndex();
            if (mxNetIndex.Defined()) {
                l3ModelValueIndex = TFullFactorIndex{
                    slice, static_cast<NFactorSlices::TFactorIndex>(mxNetIndex.GetRef())};
            }
        }
        return l3ModelValueIndex;
    }

    EFactorSlice TFactorDomain::GetReplacementSliceUglyHack(EFactorSlice slice) const
    {
        if (EFactorSlice::WEB_PRODUCTION == slice) {
            switch (GetUniverse()) {
                case EFactorUniverse::IMAGES: {
                    return EFactorSlice::IMAGES_PRODUCTION;
                }

                default: {
                    return EFactorSlice::WEB_PRODUCTION;
                }
            }
        }

        return slice;
    }

    void TFactorDomain::InitNumFactors()
    {
        if (Normal) {
            NumFactors = Borders[EFactorSlice::ALL].End;
        } else {
            for (TSliceIterator iter; iter.Valid(); iter.Next()) {
                NumFactors = Max<size_t>(NumFactors, Borders[*iter].End);
            }
        }
    }

    void TFactorDomain::InitDefaultWebBorders(ui32 count)
    {
        Borders[EFactorSlice::WEB_PRODUCTION] = TSliceOffsets(0, count);
        Borders[EFactorSlice::WEB] = TSliceOffsets(0, count);
        Borders[EFactorSlice::FORMULA] = TSliceOffsets(0, count);
        Borders[EFactorSlice::ALL] = TSliceOffsets(0, count);
        Normal = true;

        Y_ASSERT(Borders.TryToValidate());
    }

    bool TFactorDomain::operator==(const TFactorDomain& rhs) const {
        return Borders == rhs.Borders
               && NumFactors == rhs.NumFactors
               && Normal == rhs.Normal;
    }

    EFactorSlice TFactorDomain::GetChildOfSameSize(EFactorSlice slice) const
    {
//        Y_ASSERT(Normal);
        if (Y_UNLIKELY(!Normal)) {
            return EFactorSlice::COUNT;
        }

        const TSliceOffsets& offsets = Borders[slice];
        const TSlicesInfo* slicesInfo = GetSlicesInfo();

        while (!slicesInfo->IsLeaf(slice)) {
            bool found = false;
            for (TSiblingIterator iter(slice); iter.Valid(); iter.Next()) {
                if (Borders[*iter] == offsets) {
                    slice = *iter;
                    found = true;
                    break;
                }
            }
            if (!found) {
                break;
            }
        }

        return slice;
    }

    EFactorSlice TFactorDomain::GetLeafByFactorIndex(TFactorIndex index, EFactorSlice slice) const
    {
//        Y_ASSERT(Normal);
        if (Y_UNLIKELY(!Normal)) {
            return EFactorSlice::COUNT;
        }

        const TSlicesInfo* slicesInfo = GetSlicesInfo();

        if (!Borders[slice].ContainsRelative(index)) {
            return EFactorSlice::COUNT;
        }
        TFactorIndex fullIndex = Borders[slice].GetIndex(index);

        while (!slicesInfo->IsLeaf(slice)) {
            for (TSiblingIterator iter(slice); iter.Valid(); iter.Next()) {
                if (Borders[*iter].Contains(fullIndex)) {
                    slice = *iter;
                    break;
                }
            }
        }

        Y_ASSERT(Borders[slice].Contains(fullIndex));
        return slice;
    }

    const IFactorsInfo* TFactorDomain::GetSliceFactorsInfo(EFactorSlice slice) const
    {
//        Y_ASSERT(Normal);
        if (Y_UNLIKELY(!Normal)) {
            return nullptr;
        }

        EFactorSlice replacementSlice = GetReplacementSliceUglyHack(slice); // VIDEOPOISK-6428
        return GetSlicesInfo()->GetFactorsInfo(replacementSlice);
    }

    TOneFactorInfo TFactorDomain::GetFactorInfo(TFactorIndex index, EFactorSlice slice) const
    {
//        Y_ASSERT(Normal);
        if (Y_UNLIKELY(!Normal)) {
            return TOneFactorInfo();
        }

        EFactorSlice leaf = GetLeafByFactorIndex(index, slice);
        if (leaf == EFactorSlice::COUNT) {
            return TOneFactorInfo();
        }

        return TOneFactorInfo(Borders[leaf].GetRelativeIndex(index, Borders[slice]),
            GetSliceFactorsInfo(leaf));
    }

    TFactorDomain::TIterator TFactorDomain::Begin(EFactorSlice slice) const {
        Y_ASSERT(Normal);
        if (Y_UNLIKELY(!Normal)) {
            return TIterator(*this);
        }
        return TIterator(*this, slice);
    }

    TFactorDomain::TIterator TFactorDomain::End() const {
        Y_ASSERT(Normal);
        if (Y_UNLIKELY(!Normal)) {
            return TIterator(*this);
        }
        return TIterator(*this);
    }

    bool TFactorDomain::TryGetRelativeIndexByName(EFactorSlice slice, const TString& factorName, TFactorIndex& index) const {
        const TSlicesInfo* slicesInfo = GetSlicesInfo();
        Y_ASSERT(slicesInfo);
        if (Y_UNLIKELY(!slicesInfo->IsLeaf(slice))) {
            Y_ASSERT(false);
            return false;
        }

        const IFactorsInfo* factorsInfo = GetSliceFactorsInfo(slice);
        if (!factorsInfo) {
            return false;
        }
        size_t indexValue = 0;
        if (factorsInfo->GetFactorIndex(factorName.data(), &indexValue)) {
            index = indexValue;
            return true;
        }
        return false;
    }

    TFactorDomain::TIterator::TIterator(const TFactorDomain& domain, EFactorSlice slice)
        : Domain(&domain)
        , Slice(slice)
        , LeafIter(slice)
    {
        if ((*Domain)[Slice].Empty()) {
            Index = Domain->Size();
        } else {
            Index = (*Domain)[slice].Begin;
            UpdateCurrentLeaf();
            Y_ASSERT(LeafIter.Valid());
        }
    }

    void TFactorDomain::TIterator::UpdateCurrentLeaf()
    {
        while (LeafIter.Valid() && (*Domain)[*LeafIter].Empty()) {
            LeafIter.Next();
        }
        if (!LeafIter.Valid()) {
            Index = (*Domain).Size();
        }
        UpdateCurrentInfo();
    }

    void TFactorDomain::TIterator::UpdateCurrentInfo()
    {
        if (LeafIter.Valid()) {
            Info = Domain->GetSliceFactorsInfo(*LeafIter);
        } else {
            Info = nullptr;
        }
    }

    TFactorIterator TFactorDomain::TIterator::To(const TFactorDomain& domain) const
    {
        if (domain.HasIndex(*this)) {
            TFactorIterator res = *this;
            res.Domain = &domain;
            res.Index = domain.GetIndex(*this);
            return res;
        } else {
            return domain.End();
        }
    }
} // NFactorSlices
