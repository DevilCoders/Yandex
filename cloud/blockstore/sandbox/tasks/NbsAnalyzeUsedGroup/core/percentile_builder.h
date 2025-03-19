#pragma once

#include "metrics.h"

#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

struct TPercentileBuilder
{
    enum TType
    {
        READ_BYTES,
        READ_IOPS,
        WRITE_BYTES,
        WRITE_IOPS,
        //-------
        COUNT
    };

    struct TData
    {
        TVector<ui64> x;
        TVector<ui64> y;
        THashMap<ui32, int> Groups;
    };

    using TTypeToData = THashMap<TType, TData>;

    struct TPercentileData
    {
        THashMap<double, TTypeToData> percentileToData;
        TSet<ui32> groupCount;
    };

    using TKindToData = THashMap<TString, TPercentileData>;

    static TKindToData Build(const TVector<THostMetrics>& metrics);
    static TString ConverToJson(const TKindToData& metrics);
};

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
