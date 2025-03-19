#include "factor_selector.h"


void TFactorSelector::SelectByTag(const TStringBuf& tag, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value) {
    Select([&](const TOneFactorInfo& info) { return info.HasTagName(tag); }, slices, mode, value);
}

void TFactorSelector::SelectByTagId(int tag, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value) {
    Select([&](const TOneFactorInfo& info) { return info.HasTagId(tag); }, slices, mode, value);
}

void TFactorSelector::SelectByGroup(const TStringBuf& group, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value) {
    Select([&](const TOneFactorInfo& info) { return info.HasGroupName(group); }, slices, mode, value);
}

void TFactorSelector::SelectByName(const TStringBuf& name, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value) {
    Select([&](const TOneFactorInfo& info) { return name.equal(info.GetFactorName()); }, slices, mode, value);
}

void TFactorSelector::SelectByIndexRange(TFactorIndex begin, TFactorIndex end, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value) {
    Y_ASSERT(begin <= end);
    for (auto slice : slices) {
        for (TFactorIndex index = begin; index < end; ++index) {
            TFullFactorIndex fullIndex(slice, index);
            if (GetDomain().HasIndex(fullIndex)) {
                SelectFactor(fullIndex, mode, value);
            }
        }
    }
}

TFactorFiller TFactorSelector::BuildCanonicalValueFiller() const {
    size_t factorsToFillCount = 0;
    for (auto it = GetDomain().Begin(); it.Valid(); it.Next()) {
        if ((*this)[it].NeedsOverride) {
            ++factorsToFillCount;
        }
    }
    TVector<TFactorFiller::TFactorWithValue> factorsToFill;
    factorsToFill.reserve(factorsToFillCount);
    for (auto it = GetDomain().Begin(); it.Valid(); it.Next()) {
        if ((*this)[it].NeedsOverride) {
            TFullFactorIndex factorIndex = TFullFactorIndex(it.GetLeaf(), it.GetIndexInLeaf());
            float value = (*this)[factorIndex].Value.Defined() ? (*this)[factorIndex].Value.GetRef() : it.GetFactorInfo().GetCanonicalValue();
            factorsToFill.emplace_back(factorIndex, value);
        }
    }
    return { factorsToFill };
}

template <class Predicate>
void TFactorSelector::Select(const Predicate& pred, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value) {
    for (auto slice : slices) {
        for (auto it = GetDomain().Begin(slice); it.Valid(); it.Next()) {
            if (pred(it.GetFactorInfo())) {
                SelectFactor(it, mode, value);
            }
        }
    }
}

void TFactorSelector::SelectFactor(const TFullFactorIndex& factorIndex, ESelectionMode mode, const TMaybe<float>& value) {
    switch (mode) {
        case ESelectionMode::Add:
            (*this)[factorIndex].NeedsOverride = true;
            break;
        case ESelectionMode::Remove:
            (*this)[factorIndex].NeedsOverride = false;
            break;
        case ESelectionMode::Flip:
            (*this)[factorIndex].NeedsOverride ^= true;
            break;
    }

    (*this)[factorIndex].Value = value;
}
