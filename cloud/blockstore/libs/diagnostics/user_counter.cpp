#include "user_counter.h"

#include <cloud/blockstore/libs/service/request.h>


namespace NCloud::NBlockStore::NUserCounter {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TUserSumCounterWrapper
    : public IUserCounter
{
    TVector<TIntrusivePtr<NMonitoring::TCounterForPtr>> Counters;
    NMonitoring::EMetricType Type = NMonitoring::EMetricType::UNKNOWN;

    virtual NMonitoring::EMetricType GetType() const
    {
        return Type;
    }

    virtual int64_t GetValue() const
    {
        int64_t sum = 0;

        for (const auto& counter: Counters) {
            sum += counter->Val();
        }

        return sum;
    }
};

using BaseDynamicCounters = std::pair<TDynamicCounterPtr, TString>;

void AddSumWrapper(
    TUserCounterSupplier& dsc,
    const TLabels& commonlabels,
    const TVector<BaseDynamicCounters>& baseCounters,
    const TString& newName
)
{
    std::shared_ptr<TUserSumCounterWrapper> wrapper =
        std::make_shared<TUserSumCounterWrapper>();

    for (auto& counter: baseCounters)
    {
        if (counter.first)
        {
            if (auto countSub =
                counter.first->FindCounter(counter.second))
            {
                wrapper->Counters.push_back(countSub);
                wrapper->Type = countSub->ForDerivative()
                    ? EMetricType::RATE
                    : EMetricType::GAUGE;
            }
        }
    }

    if (wrapper->Type != NMonitoring::EMetricType::UNKNOWN)
    {
        dsc.AddWrapper(
            commonlabels,
            TUserCounter(
                {{"name", newName}},
                wrapper));
    }
}

auto AddSumHistogramWrapper(
    TUserCounterSupplier& dsc,
    const TLabels& commonlabels,
    const TVector<BaseDynamicCounters>& baseCounters,
    const TString& newName)
{
    static const THashMap<TString, TString> nameMap = {
        { "1ms", "1" }, { "2ms", "2" }, { "5ms", "5" },
        { "10ms", "10" }, { "20ms", "20" }, { "50ms", "50" },
        { "100ms", "100" }, { "200ms", "200" }, { "500ms", "500" },
        { "1000ms", "1000" }, { "2000ms", "2000" }, { "5000ms", "5000" },
        { "10000ms", "10000" }, { "35000ms", "35000" }, { "Inf", "inf" }
    };

    THashMap<TString, std::shared_ptr<TUserSumCounterWrapper>> wrappers;

    for (auto& counter: baseCounters)
    {
        if (counter.first)
        {
            if (auto histogram =
                counter.first->FindSubgroup("histogram", counter.second))
            {
                for (auto& [key, value]: histogram->ReadSnapshot()) {
                    auto newKey = nameMap.find(key.LabelValue);
                    if (newKey != nameMap.end()) {
                        auto countSub = histogram->GetCounter(key.LabelValue);
                        auto [wrapper, _] = wrappers.try_emplace(
                            newKey->second,
                            std::make_shared<TUserSumCounterWrapper>());
                        wrapper->second->Counters.push_back(countSub);
                        wrapper->second->Type = countSub->ForDerivative()
                            ? EMetricType::HIST_RATE
                            : EMetricType::HIST;
                    }
                }
            }
        }
    }

    for (auto& [key, wrapper]: wrappers) {
        dsc.AddWrapper(
            commonlabels,
            TUserCounter(
                {{"name", newName}, {"bin", key}},
                wrapper));
    }
}

TLabels MakeVolumeLabels(
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId)
{
    return {
        {"service", "compute"},
        {"project", cloudId},
        {"cluster", folderId},
        {"disk", diskId}};
}

TLabels MakeVolumeInstanceLabels(
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId)
{
    auto volumeLabels = MakeVolumeLabels(
        cloudId,
        folderId,
        diskId);
    volumeLabels.Add("instance", instanceId);

    return volumeLabels;
}

} //

////////////////////////////////////////////////////////////////////////////////

TUserCounter::TUserCounter(
        TLabels labels,
        std::shared_ptr<IUserCounter> counter)
    : Labels(std::move(labels))
    , Counter(std::move(counter))
{}

void TUserCounter::Accept(
    const TLabels& baseLabels,
    TInstant time,
    IMetricConsumer* consumer) const
{
    if (!Counter || !consumer) {
        return;
    }

    consumer->OnMetricBegin(Counter->GetType());

    consumer->OnLabelsBegin();

    for (const auto& label: baseLabels) {
        consumer->OnLabel(label.Name(), label.Value());
    }

    for (const auto& label: Labels) {
        consumer->OnLabel(label.Name(), label.Value());
    }

    consumer->OnLabelsEnd();

    consumer->OnInt64(time, Counter->GetValue());

    consumer->OnMetricEnd();
}

///////////////////////////////////////////////////////////////////////////////

void TUserCounterSupplier::Accept(
    TInstant time,
    IMetricConsumer* consumer) const
{
    if (!consumer) {
        return;
    }

    consumer->OnStreamBegin();
    {
        TReadGuard g{Lock};
        for (const auto& it: Metrics) {
            it.second.Accept(it.first, time, consumer);
        }
    }
    consumer->OnStreamEnd();
}

void TUserCounterSupplier::Append(
    TInstant time,
    IMetricConsumer* consumer) const
{
    TReadGuard g{Lock};
    for (const auto& it: Metrics) {
        it.second.Accept(it.first, time, consumer);
    }
}

void TUserCounterSupplier::AddWrapper(
    TLabels labels,
    TUserCounter metric)
{
    TReadGuard g{Lock};
    Metrics.emplace(std::move(labels), std::move(metric));
}

void TUserCounterSupplier::RemoveWrapper(const TLabels& labels)
{
    TWriteGuard g{Lock};
    Metrics.erase(labels);
}

////////////////////////////////////////////////////////////////////////////////

void RegisterServiceVolume(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    TDynamicCounterPtr src)
{
    AddSumWrapper(
        dsc,
        MakeVolumeLabels(cloudId, folderId, diskId),
        { { src, "UsedQuota" } },
        "disk.io_quota_utilization_percentage");
}

void UnregisterServiceVolume(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId)
{
    dsc.RemoveWrapper(
        MakeVolumeLabels(cloudId, folderId, diskId));
}

void RegisterServerVolumeInstance(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId,
    TDynamicCounterPtr src)
{
    auto commonlabels =
        MakeVolumeInstanceLabels(cloudId, folderId, diskId, instanceId);

    auto readSub = src->FindSubgroup("request", "ReadBlocks");
    AddSumWrapper(
        dsc,
        commonlabels,
        { { readSub, "Count" } },
        "disk.read_ops");
    AddSumWrapper(
        dsc,
        commonlabels,
        { { readSub, "Errors" } },
        "disk.read_errors");
    AddSumWrapper(
        dsc,
        commonlabels,
        { { readSub, "RequestBytes" } },
        "disk.read_bytes");
    AddSumHistogramWrapper(
        dsc,
        commonlabels,
        { { readSub, "Time" } },
        "disk.read_latency");
    AddSumHistogramWrapper(
        dsc,
        commonlabels,
        { { readSub, "ThrottlerDelay" } },
        "disk.read_throttler_delay");

    auto writeSub = src->FindSubgroup("request", "WriteBlocks");
    auto zeroSub = src->FindSubgroup("request", "ZeroBlocks");

    AddSumWrapper(
        dsc,
        commonlabels,
        { { writeSub, "Count" }, { zeroSub, "Count" } },
        "disk.write_ops");
    AddSumWrapper(
        dsc,
        commonlabels,
        { { writeSub, "Errors" }, { zeroSub, "Errors" } },
        "disk.write_errors");
    AddSumWrapper(
        dsc,
        commonlabels,
        { { writeSub, "RequestBytes" }, { zeroSub, "RequestBytes" } },
        "disk.write_bytes");
    AddSumHistogramWrapper(
        dsc,
        commonlabels,
        { { writeSub, "Time" }, { zeroSub, "Time" } },
        "disk.write_latency");
    AddSumHistogramWrapper(
        dsc,
        commonlabels,
        { { writeSub, "ThrottlerDelay" }, { zeroSub, "ThrottlerDelay" } },
        "disk.write_throttler_delay");
}

void UnregisterServerVolumeInstance(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId)
{
    dsc.RemoveWrapper(
        MakeVolumeInstanceLabels(cloudId, folderId, diskId, instanceId));
}

} // NCloud::NBlockStore::NUserCounter
