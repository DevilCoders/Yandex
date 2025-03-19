#pragma once

#include "factor_filler.h"

#include <kernel/factor_slices/factor_map.h>
#include <util/generic/maybe.h>

using NFactorSlices::TFactorIndex;
using NFactorSlices::TFullFactorIndex;
using NFactorSlices::TFactorMap;

namespace NDetails {
    struct TFactorOverrideValue {
        TMaybe<float> Value;
        bool NeedsOverride = {};
    };
}

class TFactorSelector
    : public TFactorMap<NDetails::TFactorOverrideValue>
{
public:
    using TFactorSlices = TVector<NFactorSlices::EFactorSlice>;

    using TFactorMap<NDetails::TFactorOverrideValue>::TFactorMap;

    enum ESelectionMode {
        Add,
        Remove,
        Flip,
    };

    void SelectByTag(const TStringBuf& tag, const TFactorSlices& slices, ESelectionMode mode = ESelectionMode::Add, const TMaybe<float>& value = {});

    void SelectByTagId(int tag, const TFactorSlices& slices, ESelectionMode mode = ESelectionMode::Add, const TMaybe<float>& value = {});

    void SelectByGroup(const TStringBuf& group, const TFactorSlices& slices, ESelectionMode mode = ESelectionMode::Add, const TMaybe<float>& value = {});

    void SelectByName(const TStringBuf& name, const TFactorSlices& slices, ESelectionMode mode = ESelectionMode::Add, const TMaybe<float>& value = {});

    void SelectByIndexRange(TFactorIndex begin, TFactorIndex end, const TFactorSlices& slices, ESelectionMode mode = ESelectionMode::Add, const TMaybe<float>& value = {});

    TFactorFiller BuildCanonicalValueFiller() const;

private:
    template <class Predicate>
    void Select(const Predicate& pred, const TFactorSlices& slices, ESelectionMode mode, const TMaybe<float>& value);

    void SelectFactor(const TFullFactorIndex& factorIndex, ESelectionMode mode, const TMaybe<float>& value);
};
