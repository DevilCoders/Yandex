#pragma once

#include "time_normalizer.h"
#include "ydb_executer.h"

#include <util/datetime/base.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

class TRateCalculator
{
private:
    const size_t ThreadCount;

    const TDuration YdbTimeFrom;
    const TDuration YdbTimeTo;

    const TDuration DataWindowTime = TDuration::Minutes(5);

    std::shared_ptr<TYDBExecuter> YdbExecuter;

    TVector<THostMetrics> SummRateData;
    TTimeNormalizer TimeNormalizer;
    THashMap<TString, TMaybe<THostMetrics>> PreviousData;

public:
    TRateCalculator(
        size_t threadCount,
        std::shared_ptr<TYDBExecuter> ydbExecuter,
        TDuration ydbTimeFrom,
        TDuration ydbTimeTo);

    TVector<THostMetrics> GetRateData(const TString& table);

private:
    TVector<TString> GetHosts(const TYDBExecuter::TRawData& rawData) const;
    void Init(const TVector<TString>& hosts);

    TVector<THostMetrics> Calculate(
        const TVector<TString>& hosts,
        TAtomic& hostIndex,
        const TYDBExecuter::TRawData& rawData);
};

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
