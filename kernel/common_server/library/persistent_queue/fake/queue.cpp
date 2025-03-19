#include "queue.h"

namespace NCS {
    TPQFake::TPQFake(const TString& clientId, const TPQFakeConfig& config, const IPQConstructionContext& /*context*/)
        : TBase(clientId)
        , Config(config) {
    }

    bool TPQFake::DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* /*failedMessages*/, const size_t maxSize, const TDuration /*timeout*/) const {
        TGuard<TMutex> g(Mutex);
        auto it = Messages.begin();
        while (it != Messages.end() && result.size() < maxSize) {
            result.emplace_back(it->second);
            ++it;
        }
        TFLEventLog::Debug()("count", result.size());
        return true;
    }

    IPQClient::TPQResult TPQFake::DoWriteMessage(const IPQMessage& message) const {
        auto pqMessageSimple = MakeHolder<TPQMessageSimple>(TBlob::Copy(message.GetContent().Data(), message.GetContent().Length()), message.GetMessageId());
        TGuard<TMutex> g(Mutex);
        Messages.emplace(message.GetMessageId(), pqMessageSimple.Release());
        TFLEventLog::Debug("write success");
        return IPQClient::TPQResult(true);
    }

    IPQClient::TPQResult TPQFake::DoAckMessage(const IPQMessage& msg) const {
        TGuard<TMutex> g(Mutex);
        if (!Messages.contains(msg.GetMessageId())) {
            TFLEventLog::Error("message already acked")("message_id", msg.GetMessageId());
            return IPQClient::TPQResult(false);
        }
        Messages.erase(msg.GetMessageId());
        return IPQClient::TPQResult(true);
    }

    bool TPQFake::DoFlushWritten() const {
        TFLEventLog::Debug("flush written success");
        return true;
    }


    bool TPQFake::IsReadable() const {
        return true;
    }


    bool TPQFake::IsWritable() const {
        return true;
    }

}
