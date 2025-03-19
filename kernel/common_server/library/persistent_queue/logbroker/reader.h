#pragma once

#include "config.h"
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kikimr/public/sdk/cpp/client/ydb_persqueue/persqueue.h>
#include <ydb/public/sdk/cpp/client/ydb_persqueue_core/persqueue.h>

namespace NCS {

    class TLogbrokerReader {
    public:
        TLogbrokerReader(const TLogbrokerClientConfig::TReadConfig& config);
        bool Start(NYdb::NPersQueue::TPersQueueClient& client, const TDuration timeout);
        bool Stop();
        bool ReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const;
        IPQClient::TPQResult AckMessage(const IPQMessage& message, const TDuration timeout) const;

    private:
        class TPartition {
        public:
            void AckAck(ui64 offset);
            NThreading::TFuture<void> RequestAck(ui64 offset);
        private:
            ui64 CommittedOffset = 0;
            TMap<ui64, NThreading::TPromise<void>> AcksInFly;
            TAdaptiveLock Lock;
        };

    private:
        template <class TEvent>
        TMaybe<TEvent> ReadUntilEvent(const TDuration timeout) const;
        TPartition& GetPartition(const NYdb::NPersQueue::TPartitionStream::TPtr& p) const;

    private:
        std::shared_ptr<NYdb::NPersQueue::IReadSession> Session;
        const TLogbrokerClientConfig::TReadConfig& Config;
        mutable TMap<TString, TPartition> Partitions;
        TAdaptiveLock PartitionsLock;
        mutable TDeque<IPQMessage::TPtr> ReadedMessages;
        TAdaptiveLock ReadLock;
    };
}
