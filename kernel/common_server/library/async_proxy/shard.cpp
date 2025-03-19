#include "shard.h"
#include "message.h"
#include "addr.h"
#include "rate.h"

#include <kernel/common_server/library/metasearch/simple/config.h>

#include <search/session/logger/logger.h>

bool TShardDelivery::IsConnectionProblem(const NNeh::TResponse* response) {
    return response && response->GetErrorType() == NNeh::TError::UnknownType &&
        (response->GetSystemErrorCode() == ECONNREFUSED || response->GetSystemErrorCode() == ETIMEDOUT);
}

ui32 TShardDelivery::GetParallelRequestsCountOnStart() const {
    return MetaConfig.GetParallelRequestCount();
}

ui32 TShardDelivery::GetMaxAttemptions() const {
    return MetaConfig.GetMaxAttempts();
}

ui32 TShardDelivery::GetMaxReconnections() const {
    return MetaConfig.GetMaxReconnections();
}

TDuration TShardDelivery::GetSwitchDuration() const {
    return MetaConfig.GetTasksCheckInterval();
}

void TShardDelivery::DoAddResponse(const NNeh::IMultiClient::TEvent& ev, const TDuration replyDuration) noexcept {
    const ui64 valueNew = Owner->AddResponse(this, ev.Type, ev.Hndl->Response()) ? 1 : 0;
    if (AtomicCas(&StopSwitchFlag, valueNew, 0) && valueNew) {
        const ISignalsManager* manager = Owner->GetSignalsManager();
        if (manager) {
            manager->DurationBackendReply(replyDuration);
        }
    }
    if ((IsConnectionProblem(ev.Hndl->Response()) || !ev.Hndl->MessageSendedCompletely()) && !AtomicGet(StopSwitchFlag)) {
        if (AtomicIncrement(AttReconnect) <= GetMaxReconnections()) {
            const ISignalsManager* manager = GetOwner()->GetSignalsManager();
            if (manager) {
                manager->SignalReaskConnection();
            }
            AtomicDecrement(AttCurrent);
            AsyncDelivery->InitReask(this);
        }
    }
}

IAddrDelivery* TShardDelivery::DoNext() {
    if (AtomicGet(StopSwitchFlag)) {
        return nullptr;
    }

    ui64 attCurrent = AtomicIncrement(AttCurrent) - 1;

    if (attCurrent + 1 > GetMaxAttemptions()) {
        return nullptr;
    }
    return DoNext(attCurrent);
}

void IShardDelivery::Acquire() {
    Owner->Acquire();
    AtomicIncrement(LinksCounter);
}

void IShardDelivery::Release() {
    AtomicDecrement(LinksCounter);
    Owner->Release();
}

IShardDelivery::~IShardDelivery() {
    CHECK_WITH_LOG(!AtomicGet(LinksCounter)) << AtomicGet(LinksCounter);
}

void IShardDelivery::AddResponse(const NNeh::IMultiClient::TEvent& ev, const TDuration replyDuration, const ui32 attNum) noexcept {
    IReaskLimiter* limiter = GetReaskLimiter();
    if (limiter) {
        limiter->OnReply(attNum != 0);
    }
    DoAddResponse(ev, replyDuration);
}

IAddrDelivery* IShardDelivery::Next() {
    IReaskLimiter* limiter = GetReaskLimiter();
    IEventLogger* logger = GetOwner()->GetReportBuilder().GetEventLogger();
    THolder<IAddrDelivery> result(DoNext());
    if (!result) {
        return nullptr;
    }

    if (result->GetAttemption() > 0) {
        const ISignalsManager* manager = GetOwner()->GetSignalsManager();
        if (limiter && !limiter->CanReask()) {
            if (manager) {
                manager->SignalRateLimited();
            }
            if (logger) {
                logger->LogEvent(NEvClass::TStageMessage(GetShardId(), "localhost", "RATELIMITED"));
            }
            return nullptr;
        } else {
            if (manager) {
                manager->SignalReask();
            }
        }
    }
    if (limiter) {
        limiter->OnRequest(result->GetAttemption());
    }
    if (logger) {
        logger->LogEvent(NEvClass::TStageMessage(GetShardId(), result->GetMessage().Addr + result->GetMessage().Data, "SENDING"));
    }
    return result.Release();
}
