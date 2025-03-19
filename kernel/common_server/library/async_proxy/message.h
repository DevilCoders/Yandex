#pragma once

#include "report.h"
#include "shard.h"

#include <kernel/common_server/util/destroyer.h>

#include <library/cpp/neh/multiclient.h>

#include <util/datetime/base.h>
#include <util/generic/object_counter.h>
#include <util/generic/vector.h>

class TUnistat;

class ISignalsManager {
private:
    TString SignalRateLimitedCTypeService;
    TString SignalRateLimitedCType;
    TString SignalReaskCTypeService;
    TString SignalReaskCType;
    TString SignalReaskConnectionCTypeService;
    TString SignalReaskConnectionCType;
    TString SignalReplyDurationCTypeService;
    TString SignalReplyDurationCType;

public:
    using TPtr = TAtomicSharedPtr<ISignalsManager>;

public:
    ISignalsManager(const TString& serviceName);
    virtual ~ISignalsManager();

    void Init(TUnistat& creator) const;
    virtual void SignalRateLimited(const ui32 count = 1) const;
    virtual void SignalReask(const ui32 count = 1) const;
    virtual void SignalReaskConnection(const ui32 count = 1) const;
    virtual void DurationBackendReply(const TDuration durationReply) const;
};

class TAsyncTask: public TObjectCounter<TAsyncTask> {
private:
    TVector<IShardDelivery::TPtr> Shards;
    IReportBuilder::TPtr ReportBuilder;
    TInstant Deadline = TInstant::Max();
    TAtomic LinksCounter = 0;
    const ISignalsManager* SignalsManager = nullptr;

public:
    using TPtr = TAtomicSharedPtr<TAsyncTask>;

public:
    TAsyncTask(IReportBuilder::TPtr reportBuilder, const TInstant deadline, const ISignalsManager* signalsManager = nullptr);

    const ISignalsManager* GetSignalsManager() const;
    TAsyncTask& SetSignalsManager(const ISignalsManager* value);

    virtual IReportBuilder& GetReportBuilder() final;
    virtual TInstant GetDeadline() const final;

    virtual ~TAsyncTask();

    Y_FORCE_INLINE void Acquire() {
        AtomicIncrement(LinksCounter);
    }

    Y_FORCE_INLINE void Release() {
        if (!AtomicDecrement(LinksCounter)) {
            Singleton<TDestroyer<TAsyncTask>>("async_task_destroyer")->Register(this);
        }
    }

    bool AddShard(IShardDelivery::TPtr shard) noexcept;
    const TVector<IShardDelivery::TPtr>& GetShards();

    virtual bool AddResponse(const IShardDelivery* shard, const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) final;
};
