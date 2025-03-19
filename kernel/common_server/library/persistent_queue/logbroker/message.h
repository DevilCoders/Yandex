#pragma once

#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kikimr/public/sdk/cpp/client/ydb_persqueue/persqueue.h>

namespace NCS {
    class TLogbrokerReadMessage: public IPQMessage {
    public:
        TLogbrokerReadMessage(NYdb::NPersQueue::TReadSessionEvent::TDataReceivedEvent::TMessage&& msg);
        virtual const TString& GetMessageId() const noexcept override;
        virtual TBlob GetContent() const noexcept override;
        NYdb::NPersQueue::TReadSessionEvent::TDataReceivedEvent::TMessage& GetLogbrokerMessage() const noexcept;

    private:
        mutable NYdb::NPersQueue::TReadSessionEvent::TDataReceivedEvent::TMessage LogbrokerMessage;
    };
}
