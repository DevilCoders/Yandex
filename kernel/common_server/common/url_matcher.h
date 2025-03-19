#pragma once
#include <kernel/common_server/util/accessor.h>
#include <util/generic/map.h>
#include <util/generic/fwd.h>
#include <util/generic/vector.h>

namespace NCS {
    class TPathSegmentInfo {
    private:
        CSA_DEFAULT(TPathSegmentInfo, TString, Variable);
        CSA_DEFAULT(TPathSegmentInfo, TString, Value);
    public:
        TPathSegmentInfo(const TString& pathSegment);
    };

    class TPathHandlerInfo {
    private:
        CSA_READONLY_DEF(TString, PathPattern);
        CSA_READONLY_DEF(TString, PathPatternCorrected);
    public:
        bool operator<(const TPathHandlerInfo& item) const {
            return PathPattern < item.PathPattern;
        }

        Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& pathPattern);
        bool Match(const TStringBuf req, ui32& templates, ui32& deep, TMap<TString, TString>& params) const;
        TVector<TPathSegmentInfo> GetPatternInfo() const;
    };

}
