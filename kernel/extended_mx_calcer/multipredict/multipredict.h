#pragma once

#include <library/cpp/scheme/scheme.h>
#include <kernel/extended_mx_calcer/proto/typedefs.h>
#include <kernel/extended_mx_calcer/interface/context.h>

namespace NExtendedMx {
    class IMultiPredict {
    public:
        virtual ~IMultiPredict() = default;

        virtual TFeatureResultConst GetBestAvailableResult(const TFeatureContextDictConstProto& featContext, const TAvailVTCombinationsConst& availVtComb, TDebug& debug) const = 0;
    };

    TSimpleSharedPtr<IMultiPredict> MakeMultiPredict(const TMultiPredictConstProto& multiPredictProto);
}
