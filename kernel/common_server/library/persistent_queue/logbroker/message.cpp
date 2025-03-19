#include "message.h"
#include <kernel/common_server/library/logging/events.h>
#include <util/generic/singleton.h>

namespace NCS {
    TLogbrokerReadMessage::TLogbrokerReadMessage(NYdb::NPersQueue::TReadSessionEvent::TDataReceivedEvent::TMessage&& msg)
        : LogbrokerMessage(std::move(msg))
    {}

    const TString& TLogbrokerReadMessage::GetMessageId() const noexcept {
        return Default<TString>();
    }

    TBlob TLogbrokerReadMessage::GetContent() const noexcept {
        return TBlob::NoCopy(LogbrokerMessage.GetData().data(), LogbrokerMessage.GetData().length());
    }

    NYdb::NPersQueue::TReadSessionEvent::TDataReceivedEvent::TMessage& TLogbrokerReadMessage::GetLogbrokerMessage() const noexcept {
        return LogbrokerMessage;
    }

}
