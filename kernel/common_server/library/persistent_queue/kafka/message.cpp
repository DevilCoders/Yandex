#include "message.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    TKafkaReadMessage::TKafkaReadMessage(cppkafka::Message&& msg)
        : KafkaMessage(std::move(msg))
        , MessageId((const char*)KafkaMessage.get_key().get_data(), KafkaMessage.get_key().get_size())
    {
    }

    const TString& TKafkaReadMessage::GetMessageId() const noexcept {
        return MessageId;
    }

    TBlob TKafkaReadMessage::GetContent() const noexcept {
        const auto& payload = KafkaMessage.get_payload();
        return TBlob::NoCopy(payload.get_data(), payload.get_size());
    }

}
