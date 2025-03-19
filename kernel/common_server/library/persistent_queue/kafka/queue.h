#pragma once

#include "config.h"

#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <contrib/libs/cppkafka/include/cppkafka/configuration.h>
#include <contrib/libs/cppkafka/include/cppkafka/topic.h>
#include <contrib/libs/cppkafka/include/cppkafka/consumer.h>
#include <contrib/libs/cppkafka/include/cppkafka/producer.h>

namespace NCS {

    class TPQKafka: public IPQClient {
    private:
        using TBase = IPQClient;
    public:
        TPQKafka(const TString& clientId, const TKafkaConfig& config, const IPQConstructionContext& context);
    protected:
        virtual bool DoStartImpl() override;
        virtual bool DoStopImpl() override;

        virtual bool DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize = Max<size_t>(), const TDuration timeout = Max<TDuration>()) const override;
        virtual IPQClient::TPQResult DoWriteMessage(const IPQMessage& message) const override;
        virtual IPQClient::TPQResult DoAckMessage(const IPQMessage& msg) const override;
        virtual bool DoFlushWritten() const override;
        virtual bool IsReadable() const override;
        virtual bool IsWritable() const override;
    private:
        const TKafkaConfig Config;
        THolder<cppkafka::Consumer> KafkaConsumer;
        THolder<cppkafka::Producer> KafkaProducer;
    };
}
