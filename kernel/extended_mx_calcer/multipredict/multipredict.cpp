#include "multipredict.h"

#include "scored_categs.h"

namespace NExtendedMx {
    TSimpleSharedPtr<IMultiPredict> MakeMultiPredict(const TMultiPredictConstProto& data) {
        if (data.Type() == "scored_categs") {
            return MakeSimpleShared<TScoredCategs>(data.Data().Value__);
        }
        return TSimpleSharedPtr<IMultiPredict>(nullptr);
    }
}
