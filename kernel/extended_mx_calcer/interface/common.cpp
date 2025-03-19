#include "common.h"

#include "context.h"

namespace NExtendedMx {
    bool FeatureValueAvailableInContext(const TFeatureContextDictConstProto& featContext, const TStringBuf& featName, const TStringBuf& featValue, TDebug& debug) {
        Y_UNUSED(debug);
        const auto& availibleValues = featContext[featName].AvailibleValues().GetRawValue()->GetDict();
        return availibleValues.empty() || (availibleValues.contains(featValue) && availibleValues.Get(featValue).IsTrue());
    }

    bool CombinationAvailableInContext(const TAvailVTCombinationsConst& availVtComb, const TStringBuf& viewType, const TStringBuf& place, ui32 pos, TDebug& debug) {
        if (availVtComb.IsNull() || availVtComb.Empty()) {
            return true;
        }
        const auto& availViewType = availVtComb[viewType];
        if (availViewType.IsNull()) {
            debug << "ban (" << viewType << "," << place << "," << pos << ") bad viewtype\n";
            return false;
        }
        const auto& availPlace = availViewType[place];
        // forbidden place
        if (availPlace.IsNull()) {
            debug << "ban (" << viewType << "," << place << "," << pos << ") bad place\n";
            return false;
        }
        // if not all positions allowed for this viewtype & place
        if (!(availPlace.IsString() && availPlace.AsString() == "*")) {
            const auto& availPoses = availPlace.AsArray<ui32>();
            if (Find(availPoses, pos) == availPoses.end()) {
                debug << "ban (" << viewType << "," << place << "," << pos << ") bad pos\n";
                return false;
            }
        }

        return true;
    }
};
