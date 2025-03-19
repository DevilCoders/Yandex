#pragma once

#include "shard.h"

#include <library/cpp/neh/multiclient.h>

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/system/event.h>

class IShardDelivery;
class IEventLogger;

class IReportBuilder {
private:
    TMap<const IShardDelivery*, ui32> RegisteredIndex;
    TSet<ui32> ShardIds;
    bool ReplyReady = false;
    TAtomic RepliesCounter = 0;
    mutable TSystemEvent EventReplyReady;

protected:
    TString UnknownErrorMessage;

protected:
    virtual bool DoAddResponse(const IShardDelivery* shard, const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) = 0;
    virtual void DoAddShardInfo(IShardDelivery* shardInfo) = 0;
    ui32 GetShardIndex(const IShardDelivery* shard) const;
    virtual void DoOnReplyReady() const = 0;

    virtual void OnReplyReady() final;

public:
    using TPtr = TAtomicSharedPtr<IReportBuilder>;

public:
    virtual ~IReportBuilder();

    void WaitReply(const TInstant deadline) const {
        EventReplyReady.WaitD(deadline);
    }

    virtual bool CheckReply() final;

    void SetError(const TString& error) {
        UnknownErrorMessage = error;
    }

    bool HasErrors() const {
        return !UnknownErrorMessage.empty();
    }

    virtual void AddShardInfo(IShardDelivery* shardInfo) final;
    virtual bool AddResponse(const IShardDelivery* shard, const NNeh::IMultiClient::TEvent::TType evType, const NNeh::TResponse* response) final;

    virtual IEventLogger* GetEventLogger() const {
        return nullptr;
    }
};
