#pragma once

#include "metrics.h"

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/system/mutex.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

class TTimeNormalizer
{
private:
    struct TTimeCache
    {
        i64 TimeOffset = 0;
        i64 PreviousTime = 0;
    };

private:
    const i64 WindowTime;
    const i64 AllowableFaultTime;

    THashMap<TString, TTimeCache> HostsToTimeCache;

public:
    TTimeNormalizer(
        TDuration windowTime,
        TDuration allowableFaultTime);

    void Init(const TVector<TString>& hosts);
    void Normalize(const TString& host, THostMetrics& metrics);
};

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
