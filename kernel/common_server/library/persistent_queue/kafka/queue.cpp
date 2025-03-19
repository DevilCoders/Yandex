#include "queue.h"
#include <library/cpp/logger/global/global.h>
#include "message.h"

namespace NCS {

    TPQKafka::TPQKafka(const TString& clientId, const TKafkaConfig& config, const IPQConstructionContext& /*context*/)
        : TBase(clientId)
        , Config(config) {
    }

    bool TPQKafka::DoStartImpl() {
        if (Config.HasReadConfig()) {
            KafkaConsumer = MakeHolder<cppkafka::Consumer>(Config.GetReadConfigUnsafe().BuildKafkaConfig());
            if (auto p = Config.GetPartitionMaybe()) {
                KafkaConsumer->assign({ cppkafka::TopicPartition(Config.GetTopic(), *p) });
            } else {
                KafkaConsumer->subscribe({ Config.GetTopic() });
            }
        }
        if (Config.HasWriteConfig()) {
            KafkaProducer = MakeHolder<cppkafka::Producer>(Config.GetWriteConfigUnsafe().BuildKafkaConfig());
        }
        return true;
    }

    bool TPQKafka::DoStopImpl() {
        if (KafkaProducer) {
            KafkaProducer.Destroy();
        }

        if (KafkaConsumer) {
            KafkaConsumer->unsubscribe();
            KafkaConsumer->unassign();
            KafkaConsumer.Destroy();
        }
        return true;
    }

    bool TPQKafka::DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize /*= Max<size_t>()*/, const TDuration timeout /*= Max<TDuration>()*/) const {
        auto msgs = KafkaConsumer->poll_batch(maxSize, std::chrono::milliseconds(timeout.MilliSeconds()));
        for (auto&& msg : msgs) {
            if (auto error = msg.get_error()) {
                if (failedMessages) {
                    ++*failedMessages;
                }
                TFLEventLog::Log("Error while read messages", TLOG_ERR)("exception", error.to_string());
            } else {
                result.emplace_back(MakeAtomicShared<TKafkaReadMessage>(std::move(msg)));
            }
        }
        return true;
    }

    IPQClient::TPQResult TPQKafka::DoWriteMessage(const IPQMessage& message) const {
        cppkafka::MessageBuilder builder(Config.GetTopic());
        const auto& data = message.GetContent();
        const auto& key = message.GetMessageId();
        builder
            .payload(cppkafka::Buffer(data.AsCharPtr(), data.Size()))
            .key(cppkafka::Buffer(key.Data(), key.Size()));

        if (Config.HasPartition()) {
            builder.partition(Config.GetPartitionUnsafe());
        }

        KafkaProducer->produce(std::move(builder));
        return IPQClient::TPQResult(true);
    }

    IPQClient::TPQResult TPQKafka::DoAckMessage(const IPQMessage& msg) const {
        const TKafkaReadMessage* kfkMsg = dynamic_cast<const TKafkaReadMessage*>(&msg);
        if (!kfkMsg) {
            TFLEventLog::Error("incorrect message class");
            return IPQClient::TPQResult(false);
        }
        KafkaConsumer->commit(kfkMsg->GetKafkaMessage());
        return IPQClient::TPQResult(true);
    }

    bool TPQKafka::DoFlushWritten() const {
        KafkaProducer->flush();
        return true;
    }

    bool TPQKafka::IsReadable() const {
        return !!KafkaConsumer;
    }

    bool TPQKafka::IsWritable() const {
        return !!KafkaProducer;
    }

}
