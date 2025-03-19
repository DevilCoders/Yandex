#pragma once

#include "slices_info.h"

#include <kernel/factors_info/factors_info.h>

#include <kernel/factor_slices/factor_slices_gen.h>

namespace NFactorSlices {
    class TSlicesMetaInfo
        : public TSlicesMetaInfoBase
    {
    public:
        void Merge(const TSlicesMetaInfo& other);
    };

    class TGlobalSlicesMetaInfo
        : public TSlicesMetaInfo
    {
    public:
        static TGlobalSlicesMetaInfo& Instance();

        void SetFactorsInfo(EFactorSlice slice, const IFactorsInfo* info);
        const IFactorsInfo* GetFactorsInfo(EFactorSlice slice) const {
            return Infos[slice];
        }

    private:
        TSliceMap<const IFactorsInfo*> Infos;
    };

    Y_FORCE_INLINE void EnableSlices(TSlicesMetaInfo&) {
    }

    template <typename... Args>
    void EnableSlices(TSlicesMetaInfo& metaInfo, EFactorSlice slice, Args... other) {
        auto info = GetSlicesInfo();
        for (; slice != EFactorSlice::COUNT; slice = info->GetParent(slice)) {
            metaInfo.SetSliceEnabled(slice, true);
        }
        EnableSlices(metaInfo, other...);
    }
    template <typename SliceCont, typename... Args>
    void EnableSlices(TSlicesMetaInfo& metaInfo, const SliceCont& slices, Args... other) {
        for (EFactorSlice slice : slices) {
            EnableSlices(metaInfo, slice);
        }
        EnableSlices(metaInfo, other...);
    }

    Y_FORCE_INLINE void DisableSlices(TSlicesMetaInfo&) {
    }

    template <typename... Args>
    void DisableSlices(TSlicesMetaInfo& metaInfo, EFactorSlice slice, Args... other) {
        metaInfo.SetSliceEnabled(slice, false);
        DisableSlices(metaInfo, other...);
    }
    template <typename SliceCont, typename... Args>
    void DisableSlices(TSlicesMetaInfo& metaInfo, const SliceCont& slices, Args... other) {
        for (EFactorSlice slice : slices) {
            metaInfo.SetSliceEnabled(slice, false);
        }
        DisableSlices(metaInfo, other...);
    }
} // NFactorSlices
