#pragma once

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/monlib/metrics/metric_registry.h>

namespace NCloud::NBlockStore::NUserCounter {

////////////////////////////////////////////////////////////////////////////////

class IUserCounter
{
public:
    virtual ~IUserCounter() = default;

    virtual NMonitoring::EMetricType GetType() const = 0;
    virtual int64_t GetValue() const = 0;
};

class TUserCounter
{
private:
    const NMonitoring::TLabels Labels;
    std::shared_ptr<IUserCounter> Counter;

public:
    TUserCounter(
        NMonitoring::TLabels labels,
        std::shared_ptr<IUserCounter> counter);

    void Accept(
        const NMonitoring::TLabels& baseLabels,
        TInstant time,
        NMonitoring::IMetricConsumer* consumer) const;
};

////////////////////////////////////////////////////////////////////////////////

class TUserCounterSupplier
    : public NMonitoring::IMetricSupplier
{
private:
    TRWMutex Lock;
    THashMultiMap<NMonitoring::TLabels, TUserCounter> Metrics;

public:
    // NMonitoring::IMetricSupplier
    void Accept(
        TInstant time,
        NMonitoring::IMetricConsumer* consumer) const override;
    void Append(
        TInstant time,
        NMonitoring::IMetricConsumer* consumer) const override;

    void AddWrapper(NMonitoring::TLabels labels, TUserCounter metric);
    void RemoveWrapper(const NMonitoring::TLabels& labels);
};

////////////////////////////////////////////////////////////////////////////////

void RegisterServiceVolume(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    NMonitoring::TDynamicCounterPtr src);

void UnregisterServiceVolume(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId);

void RegisterServerVolumeInstance(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId,
    NMonitoring::TDynamicCounterPtr src);

void UnregisterServerVolumeInstance(
    TUserCounterSupplier& dsc,
    const TString& cloudId,
    const TString& folderId,
    const TString& diskId,
    const TString& instanceId);

} // NCloud::NBlockStore::NUserCounter
