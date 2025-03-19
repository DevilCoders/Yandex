#pragma once

#include "config.h"
#include "writer.h"
#include "reader.h"
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <kikimr/public/sdk/cpp/client/ydb_persqueue/persqueue.h>

namespace NCS {

    class TLogbrokerClient final: public IPQClient {
    private:
        using TBase = IPQClient;
    public:
        TLogbrokerClient(const TString& clientId, const TLogbrokerClientConfig& config, const IPQConstructionContext& context);
        using TPtr = TAtomicSharedPtr<TLogbrokerClient>;

    protected:
        virtual bool DoStartImpl() override;
        virtual bool DoStopImpl() override;
        virtual bool DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const override;
        virtual IPQClient::TPQResult DoAckMessage(const IPQMessage& message) const override;
        virtual IPQClient::TPQResult DoWriteMessage(const IPQMessage& message) const  override;
        virtual bool DoFlushWritten() const override;
        virtual bool IsReadable() const override;
        virtual bool IsWritable() const override;

    private:
        const TLogbrokerClientConfig& Config;
        const IPQConstructionContext& ConstructionContext;
        THolder<NYdb::TDriver> Driver;
        THolder<NYdb::NPersQueue::TPersQueueClient> PQClient;
        THolder<TLogbrokerReader> Reader;
        THolder<TLogbrokerWriter> Writer;
    };
}
