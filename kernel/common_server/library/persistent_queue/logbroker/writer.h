#pragma once

#include "config.h"
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kikimr/public/sdk/cpp/client/ydb_persqueue/persqueue.h>
#include <ydb/public/sdk/cpp/client/ydb_persqueue_core/persqueue.h>
#include <library/cpp/threading/future/future.h>

namespace NCS {

    class TLogbrokerWriter {
    public:
        TLogbrokerWriter(const TLogbrokerClientConfig::TWriteConfig& config);
        bool Start(NYdb::NPersQueue::TPersQueueClient& client);
        bool Stop();
        IPQClient::TPQResult WriteMessage(const IPQMessage& message, const TDuration timeout);
        bool FlushWritten(const TDuration timeout);

    private:
        struct TWriteMessage {
            TWriteMessage(const ui64 seqNo, const TBlob& content);
            bool IsEarlerThen(const ui64 seqNo) const;

            ui64 SeqNo;
            TBlob Content;
            NThreading::TPromise<void> Acked;
        };

    private:
        const TLogbrokerClientConfig::TWriteConfig& Config;
        std::shared_ptr<NYdb::NPersQueue::IWriteSession> Session;
        TMutex ContentBufferMutex;
        THolder<TThreadPool> WriteThread;
        TDeque<TWriteMessage> ContentBuffer;
        ui64 LastWrittenSeqNo = 0;
    };

}
