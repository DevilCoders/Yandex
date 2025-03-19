#pragma once

#include <library/cpp/neh/multiclient.h>

#include <util/generic/ptr.h>

class TAsyncTask;
class IAddrDelivery;
class IReaskLimiter;
class IShardDelivery;

namespace NScatter {
    class ISource;
}

namespace NSimpleMeta {
    class TConfig;
}

class IAsyncDelivery {
public:
    virtual ~IAsyncDelivery() {
    }

    virtual bool InitReask(IShardDelivery* shard) = 0;
};

class IShardDelivery {
private:
    TAtomic LinksCounter = 0;
    ui32 ShardId = Max<ui32>();
    NScatter::ISource* Source;

protected:
    TAsyncTask* Owner;
    IReaskLimiter* ReaskLimiter = nullptr;
    IAsyncDelivery* AsyncDelivery;

protected:
    virtual IAddrDelivery* DoNext() = 0;

public:
    using TPtr = TAtomicSharedPtr<IShardDelivery>;

public:
    IShardDelivery(TAsyncTask* owner, NScatter::ISource* source, IAsyncDelivery* asyncDelivery, const ui32 shardId)
        : ShardId(shardId)
        , Source(source)
        , Owner(owner)
        , AsyncDelivery(asyncDelivery)
    {
    }

    IShardDelivery& SetReasksLimiter(IReaskLimiter* reaskLimiter) {
        ReaskLimiter = reaskLimiter;
        return *this;
    }

    virtual ~IShardDelivery();

    void Acquire();
    void Release();

    TAsyncTask* GetOwner() {
        return Owner;
    }

    const NScatter::ISource* GetSource() const {
        return Source;
    }

    virtual ui32 GetShardId() const final {
        return ShardId;
    }

    virtual NNeh::TMessage BuildMessage(const ui32 att) const = 0;

    virtual bool Prepare() = 0;
    virtual ui32 GetParallelRequestsCountOnStart() const = 0;
    virtual ui32 GetMaxAttemptions() const = 0;
    virtual ui32 GetMaxReconnections() const = 0;
    virtual IReaskLimiter* GetReaskLimiter() const final {
        return ReaskLimiter;
    }
    virtual TDuration GetSwitchDuration() const = 0;
    virtual void DoAddResponse(const NNeh::IMultiClient::TEvent& ev, const TDuration replyDuration) noexcept = 0;
    virtual void AddResponse(const NNeh::IMultiClient::TEvent& ev, const TDuration replyDuration, const ui32 /*attNum*/) noexcept final;
    virtual IAddrDelivery* Next() final;
};

class TShardDelivery: public IShardDelivery {
private:
    TAtomic AttCurrent = 0;
    TAtomic AttReconnect = 0;
    TAtomic StopSwitchFlag = 0;
    const NSimpleMeta::TConfig& MetaConfig;

protected:
    virtual IAddrDelivery* DoNext(const ui64 att) = 0;
    virtual IAddrDelivery* DoNext() override final;

public:
    static bool IsConnectionProblem(const NNeh::TResponse* response);

public:
    TShardDelivery(TAsyncTask* owner, NScatter::ISource* source, const NSimpleMeta::TConfig& metaConfig, IAsyncDelivery* asyncDelivery, const ui32 shardId)
        : IShardDelivery(owner, source, asyncDelivery, shardId)
        , MetaConfig(metaConfig)
    {
    }

    virtual ui32 GetParallelRequestsCountOnStart() const override;
    virtual ui32 GetMaxAttemptions() const override;
    virtual ui32 GetMaxReconnections() const override;
    virtual TDuration GetSwitchDuration() const override;

    virtual void DoAddResponse(const NNeh::IMultiClient::TEvent& ev, const TDuration replyDuration) noexcept override;
};
