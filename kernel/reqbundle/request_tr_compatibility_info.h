#pragma once

#include <library/cpp/wordpos/wordpos.h>
#include <ysite/yandex/posfilter/filter_tree.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NReqBundle {

    struct TRequestTrCompatibilityInfo {
        TVector<ui64> MainPartsWordMasks;
        TVector<EFormClass> MarkupPartsBestFormClasses;
        ui32 WordCount = 0;
        TMaybe<TTopAndArgsForWeb> TopAndArgsForWeb;
    };

} // namespace NReqBundle
