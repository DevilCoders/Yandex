#pragma once

#include "feature_slice.h"

#include <kernel/feature_pool/proto/feature_pool.pb.h>

#include <library/cpp/json/json_value.h>

namespace NMLPool {
    class TFeatureInfo;
    class TPoolInfo;
}

namespace NMLPool {
    bool HasTag(const TFeatureInfo& info, const TString& name);

    TString GetBordersStr(const TFeatureSlice& slice);
    TString GetBordersStr(const TFeatureSlices& slices);

    // This function will fail, if bordersStr has more than one slice (or any trailing chars)
    bool TryParseBordersStr(const TStringBuf bordersStr, TFeatureSlice& slice);
    bool TryParseBordersStr(const TStringBuf bordersStr, TFeatureSlices& slices);

    // This function may throw, if
    // pool has slices info that appears
    // broken
    TFeatureSlices GetSlices(const TPoolInfo& info);

    inline TString GetBordersStr(const TPoolInfo& info) {
        return GetBordersStr(GetSlices(info));
    }

    NJson::TJsonValue ToJson(const TFeatureInfo& info,
        bool extInfo = false);
    NJson::TJsonValue ToJson(const TPoolInfo& info,
        bool extInfo = false);
} // NMLPool
