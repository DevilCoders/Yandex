#pragma once

#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <contrib/libs/cppkafka/include/cppkafka/message.h>
#include <contrib/libs/cppkafka/include/cppkafka/consumer.h>

namespace NCS {
    class TKafkaReadMessage: public IPQMessage {
    public:
        TKafkaReadMessage(cppkafka::Message&& msg);
        virtual const TString& GetMessageId() const noexcept override;
        virtual TBlob GetContent() const noexcept override;
        const cppkafka::Message& GetKafkaMessage() const {
            return KafkaMessage;
        }
    private:
        cppkafka::Message KafkaMessage;
        const TString MessageId;
    };
}
