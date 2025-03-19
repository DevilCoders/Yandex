#pragma once

#include <kernel/factors_info/factors_info.h>

#include <kernel/factor_slices/factor_slices_gen.h>

namespace NFactorSlices {
    class TSlicesInfo {
    public:
        TStringBuf GetName(EFactorSlice slice) const;
        TStringBuf GetCppName(EFactorSlice slice) const;

        const IFactorsInfo* GetFactorsInfo(EFactorSlice slice) const;

        bool IsHierarchical(EFactorSlice slice) const;
        bool IsLeaf(EFactorSlice slice) const;
        bool HasOneChild(EFactorSlice slice) const;

        EFactorSlice GetParent(EFactorSlice slice) const;
        EFactorSlice GetNext(EFactorSlice slice) const;
        EFactorSlice GetNextSibling(EFactorSlice slice) const;
        EFactorSlice GetFirstChild(EFactorSlice slice) const;
        EFactorSlice GetFirstLeaf(EFactorSlice slice) const;
        EFactorSlice GetNextLeaf(EFactorSlice slice) const;

        bool IsChild(EFactorSlice parent, EFactorSlice slice) const;

        EFactorSlice GetSliceFor(EFactorUniverse universe, ESliceRole role) const;
    };

    const TSlicesInfo* GetSlicesInfo();
};
