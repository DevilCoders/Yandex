#pragma once

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/system/types.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

struct THostMetrics
{
    struct TValue
    {
        ui64 ByteCount = 0;
        ui64 Iops      = 0;
    };

    struct TLoadData
    {
        TValue ReadOperations;
        TValue WriteOperations;
    };

    using TGroupMap = THashMap<ui32, TLoadData>;
    using TKindMap = THashMap<TString, TGroupMap>;

    TDuration Timestamp;
    TKindMap PoolKind2LoadData;
};

THostMetrics ConvertToData(const TString& currentData);

THostMetrics ConvertoToRate(
    const THostMetrics& previousData,
    const THostMetrics& currentData);

void Append(
    THostMetrics& dst,
    const THostMetrics& src);

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
