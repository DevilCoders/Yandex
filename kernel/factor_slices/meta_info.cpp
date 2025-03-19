#include "meta_info.h"
#include "factor_borders.h"
#include "slice_iterator.h"
#include "slices_info.h"

#include <util/generic/singleton.h>

namespace NFactorSlices {
    void TSlicesMetaInfo::Merge(const TSlicesMetaInfo& other)
    {
        auto info = GetSlicesInfo();

        for (TSliceIterator iter; iter.Valid(); iter.Next()) {
            EFactorSlice slice = *iter;
            bool enabled = IsSliceEnabled(slice) || other.IsSliceEnabled(slice);
            bool initialized = IsSliceInitialized(slice) || other.IsSliceInitialized(slice);

            size_t numFactors = 0;
            if (IsSliceInitialized(slice)) {
                numFactors = Max<size_t>(numFactors, GetNumFactors(slice));
            }
            if (other.IsSliceInitialized(slice)) {
                numFactors = Max<size_t>(numFactors, other.GetNumFactors(slice));
            }

            if (initialized && info->IsLeaf(slice)) {
                SetNumFactors(slice, numFactors);
            }
            SetSliceEnabled(slice, enabled);
        }
    }

    TGlobalSlicesMetaInfo& TGlobalSlicesMetaInfo::Instance()
    {
        return *Singleton<TGlobalSlicesMetaInfo>();
    }

    void TGlobalSlicesMetaInfo::SetFactorsInfo(EFactorSlice slice, const IFactorsInfo* info)
    {
        Y_ASSERT(!Infos[slice]); // should be called once
        Infos[slice] = info;
    }
} // NFactorSlices
