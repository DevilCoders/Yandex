#include "rate_calculator.h"

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

TRateCalculator::TRateCalculator(
        size_t threadCount,
        std::shared_ptr<TYDBExecuter> ydbExecuter,
        TDuration ydbTimeFrom,
        TDuration ydbTimeTo)
    : ThreadCount(threadCount)
    , YdbTimeFrom(ydbTimeFrom)
    , YdbTimeTo(ydbTimeTo)
    , YdbExecuter(std::move(ydbExecuter))
    , SummRateData((YdbTimeTo - YdbTimeFrom) / DataWindowTime + 1)
    , TimeNormalizer(DataWindowTime, TDuration::Seconds(5))
{
    TDuration ydbWindowTimeTo = YdbTimeFrom;

    for (auto& sumData: SummRateData) {
        sumData.Timestamp = ydbWindowTimeTo;
        sumData.PoolKind2LoadData = {};
        ydbWindowTimeTo += DataWindowTime;
    }
}

TVector<THostMetrics> TRateCalculator::GetRateData(const TString& table)
{
    const TDuration ydbWindowTime = YdbTimeTo - YdbTimeFrom;
    TDuration ydbWindowTimeFrom = YdbTimeFrom;
    TDuration ydbWindowTimeTo = ydbWindowTimeFrom + ydbWindowTime;

    TMutex summRateDataMutex;
    while(ydbWindowTimeTo <= YdbTimeTo) {
        TAtomic hostIndex = 0;
        auto rawData = YdbExecuter->GetAllData(
            table,
            ydbWindowTimeFrom,
            ydbWindowTimeTo);

        if (rawData.empty()) {
            break;
        }

        const TVector<TString> hosts = GetHosts(rawData);
        Init(hosts);

        auto threadFunc = [&, this] {
            auto currentSummRateData = Calculate(
                hosts,
                hostIndex,
                rawData);

            TGuard<TMutex> lock(summRateDataMutex);
            for (size_t i = 0; i < currentSummRateData.size(); ++i) {
                Append(SummRateData[i], currentSummRateData[i]);
            }
        };

        TThreadPool threadPool;
        threadPool.Start(ThreadCount);
        for (size_t i = 0; i < ThreadCount; ++i) {
            threadPool.SafeAddFunc(threadFunc);
        }
        threadPool.Stop();

        ydbWindowTimeFrom = ydbWindowTimeTo;
        ydbWindowTimeTo += ydbWindowTime;
    }

    return SummRateData;
}

TVector<TString> TRateCalculator::GetHosts(
    const TYDBExecuter::TRawData& rawData) const
{
    TVector<TString> result;
    result.reserve(rawData.size());
    for (const auto& [host, data]: rawData) {
        result.push_back(host);
    }
    return result;
}

void TRateCalculator::Init(const TVector<TString>& hosts)
{
    TimeNormalizer.Init(hosts);
    for (const auto& host: hosts) {
        PreviousData.try_emplace(host, TMaybe<THostMetrics>{});
    }
}

TVector<THostMetrics> TRateCalculator::Calculate(
    const TVector<TString>& hosts,
    TAtomic& hostIndex,
    const TYDBExecuter::TRawData& rawData)
{
    TVector<THostMetrics> currentSummRateData(SummRateData.size());

    for (size_t currentHostIndex = AtomicGetAndIncrement(hostIndex);
        currentHostIndex < hosts.size();
        currentHostIndex = AtomicGetAndIncrement(hostIndex))
    {
        const TString& host = hosts[currentHostIndex];
        const auto& hostRawData = rawData.find(host);
        if (hostRawData == rawData.end()) {
            continue;
        }

        TVector<THostMetrics> normalizeData;
        for (const auto& hostData: hostRawData->second) {
            THostMetrics covertedData =
                ConvertToData(hostData.loadData);
            covertedData.Timestamp =
                TDuration::Seconds(hostData.timestamp);
            TimeNormalizer.Normalize(host, covertedData);
            normalizeData.push_back(std::move(covertedData));
        }

        for (size_t i = 0; i < normalizeData.size(); ++i) {
            THostMetrics rateData;
            if (i == 0) {
                if (PreviousData[host]) {
                    rateData = ConvertoToRate(
                        *PreviousData[host],
                        normalizeData[i]);
                }
                PreviousData[host] = normalizeData.back();
            } else {
                rateData = ConvertoToRate(
                    normalizeData[i - 1],
                    normalizeData[i]);
            }

            if (rateData.Timestamp) {
                const int index =
                    (rateData.Timestamp - YdbTimeFrom) / DataWindowTime;
                Append(currentSummRateData[index], rateData);
            }
        }
    }

    return currentSummRateData;
}

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
