#include "message.h"

#include <kernel/common_server/library/signals/intervals.h>

#include <library/cpp/unistat/unistat.h>

TAsyncTask::TAsyncTask(IReportBuilder::TPtr reportBuilder, const TInstant deadline, const ISignalsManager* signalsManager /*= nullptr*/)
    : ReportBuilder(reportBuilder)
    , Deadline(deadline)
    , SignalsManager(signalsManager)
{
}

const ISignalsManager* TAsyncTask::GetSignalsManager() const {
    return SignalsManager;
}

TAsyncTask& TAsyncTask::SetSignalsManager(const ISignalsManager* value) {
    SignalsManager = value;
    return *this;
}

IReportBuilder& TAsyncTask::GetReportBuilder() {
    return *ReportBuilder;
}

TInstant TAsyncTask::GetDeadline() const {
    return Deadline;
}

TAsyncTask::~TAsyncTask() {
    ReportBuilder->CheckReply();
}

bool TAsyncTask::AddShard(IShardDelivery::TPtr shard) noexcept {
    if (shard->Prepare()) {
        ReportBuilder->AddShardInfo(shard.Get());
        Shards.push_back(shard);
        return true;
    }
    return false;
}

const TVector<IShardDelivery::TPtr>& TAsyncTask::GetShards() {
    return Shards;
}

bool TAsyncTask::AddResponse(const IShardDelivery* shard, const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) {
    return ReportBuilder->AddResponse(shard, evType, response);
}


ISignalsManager::ISignalsManager(const TString& serviceName)
    : SignalRateLimitedCTypeService("search-" + serviceName + "-rate_limited")
    , SignalRateLimitedCType("search-rate_limited")
    , SignalReaskCTypeService("search-" + serviceName + "-reasks")
    , SignalReaskCType("search-reasks")
    , SignalReaskConnectionCTypeService("search-" + serviceName + "-reasks_connection")
    , SignalReaskConnectionCType("search-reasks_connection")
    , SignalReplyDurationCTypeService("search-" + serviceName + "-reply_duration")
    , SignalReplyDurationCType("search-reply_duration")
{
}

ISignalsManager::~ISignalsManager() {
}

void ISignalsManager::Init(TUnistat& creator) const {
    creator.DrillFloatHole(SignalRateLimitedCTypeService, "dmmm", NUnistat::TPriority(50));
    creator.DrillFloatHole(SignalRateLimitedCType, "dmmm", NUnistat::TPriority(50));
    creator.DrillFloatHole(SignalReaskCTypeService, "dmmm", NUnistat::TPriority(50));
    creator.DrillFloatHole(SignalReaskCType, "dmmm", NUnistat::TPriority(50));
    creator.DrillFloatHole(SignalReaskConnectionCTypeService, "dmmm", NUnistat::TPriority(50));
    creator.DrillFloatHole(SignalReaskConnectionCType, "dmmm", NUnistat::TPriority(50));
    creator.DrillHistogramHole(SignalReplyDurationCTypeService, "dhhh", NUnistat::TPriority(50), NRTYSignals::DefaultTimeIntervals);
    creator.DrillHistogramHole(SignalReplyDurationCType, "dhhh", NUnistat::TPriority(50), NRTYSignals::DefaultTimeIntervals);
}

void ISignalsManager::SignalRateLimited(const ui32 count) const {
    TUnistat::Instance().PushSignalUnsafe(SignalRateLimitedCType, count);
    TUnistat::Instance().PushSignalUnsafe(SignalRateLimitedCTypeService, count);
}

void ISignalsManager::SignalReask(const ui32 count /*= 1*/) const {
    TUnistat::Instance().PushSignalUnsafe(SignalReaskCTypeService, count);
    TUnistat::Instance().PushSignalUnsafe(SignalReaskCType, count);
}

void ISignalsManager::DurationBackendReply(const TDuration durationReply) const {
    TUnistat::Instance().PushSignalUnsafe(SignalReplyDurationCTypeService, durationReply.MicroSeconds() / 1000.0);
    TUnistat::Instance().PushSignalUnsafe(SignalReplyDurationCType, durationReply.MicroSeconds() / 1000.0);
}

void ISignalsManager::SignalReaskConnection(const ui32 count /*= 1*/) const {
    TUnistat::Instance().PushSignalUnsafe(SignalReaskConnectionCTypeService, count);
    TUnistat::Instance().PushSignalUnsafe(SignalReaskConnectionCType, count);
}
