#pragma once

#include "distance.h"

#include <util/system/types.h>

namespace NKNNIndex {
    template <class FeatureType, EDistanceType distanceType>
    class TSearchItemResult {
    public:
        using TFeatureType = FeatureType;
        using TDistance = typename TDistanceCalculator<FeatureType, distanceType>::TResult;

        TSearchItemResult(ui32 documentId, ui32 itemId, TDistance distance)
            : DocumentId_(documentId)
            , ItemId_(itemId)
            , Distance_(distance)
        {
        }

        ui32 DocumentId() const {
            return DocumentId_;
        }

        ui32 ItemId() const {
            return ItemId_;
        }

        TDistance Distance() const {
            return Distance_;
        }

    private:
        ui32 DocumentId_ = 0;
        ui32 ItemId_ = 0;
        TDistance Distance_ = 0;
    };

}
