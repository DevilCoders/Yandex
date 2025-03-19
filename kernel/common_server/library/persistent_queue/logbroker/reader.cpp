#include "reader.h"
#include "message.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {

    template <class TEvent>
    TMaybe<TEvent> TLogbrokerReader::ReadUntilEvent(const TDuration timeout) const {
        const auto deadline = timeout.ToDeadLine();
        while (Now() < deadline) {
            if (auto event = Session->GetEvent()) {
                if (auto* result = std::get_if<TEvent>(event.Get())) {
                    return *result;
                } else if (std::get_if<NYdb::NPersQueue::TSessionClosedEvent>(event.Get())) {
                    return Nothing();
                }
            }
            Session->WaitEvent().Wait(deadline);
        }
        return Nothing();
    }

    NCS::TLogbrokerReader::TPartition& TLogbrokerReader::GetPartition(const NYdb::NPersQueue::TPartitionStream::TPtr& p) const {
        auto g = Guard(PartitionsLock);
        return Partitions[p->GetTopicPath() + "/" + ToString(p->GetPartitionId())];
    }

    TLogbrokerReader::TLogbrokerReader(const TLogbrokerClientConfig::TReadConfig& config)
        : Config(config)
    {}

    bool TLogbrokerReader::Start(NYdb::NPersQueue::TPersQueueClient& client, const TDuration /*timeout*/) {
        auto sessionConfig = Config.BuildSessionConfig();
        NYdb::NPersQueue::TReadSessionSettings::TEventHandlers handlers;
        handlers.CommitAcknowledgementHandler([this](const NYdb::NPersQueue::TReadSessionEvent::TCommitAcknowledgementEvent& event) {
            GetPartition(event.GetPartitionStream()).AckAck(event.GetCommittedOffset());
        });

        handlers.DestroyPartitionStreamHandler([](NYdb::NPersQueue::TReadSessionEvent::TDestroyPartitionStreamEvent& event) {
            event.Confirm();
        });

        handlers.CreatePartitionStreamHandler([this](NYdb::NPersQueue::TReadSessionEvent::TCreatePartitionStreamEvent& event) {
            GetPartition(event.GetPartitionStream()).AckAck(event.GetCommittedOffset());
            event.Confirm();
        });

        sessionConfig.EventHandlers(handlers);
        if (!(Session = client.CreateReadSession(sessionConfig))) {
            TFLEventLog::Error("Cannot create read section");
            return false;
        }
        return true;
    }


    bool TLogbrokerReader::Stop() {
        if (Session && !Session->Close()) {
            TFLEventLog::Error("close Consumer failed");
            return false;
        }
        Session.reset();
        return true;
    }


    bool TLogbrokerReader::ReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const {
        result.reserve(maxSize);
        with_lock(ReadLock) {
            while (!ReadedMessages.empty()) {
                result.emplace_back(ReadedMessages.front());
                ReadedMessages.pop_front();
                if (result.size() >= maxSize) {
                    return true;
                }
            }
        }
        if (auto dataEvent = ReadUntilEvent<NYdb::NPersQueue::TReadSessionEvent::TDataReceivedEvent>(timeout)) {
            for (auto&& msg : dataEvent->GetMessages()) {
                if (msg.HasException()) {
                    if (failedMessages) {
                        ++(*failedMessages);
                    }
                    TFLEventLog::Error(msg.DebugString());
                } else {
                    if (result.size() < maxSize) {
                        result.emplace_back(MakeAtomicShared<TLogbrokerReadMessage>(std::move(msg)));
                    } else {
                        auto g = Guard(ReadLock);
                        ReadedMessages.emplace_back(MakeAtomicShared<TLogbrokerReadMessage>(std::move(msg)));
                    }
                }
            }
        }
        return true;
    }

    IPQClient::TPQResult TLogbrokerReader::AckMessage(const IPQMessage& message, const TDuration timeout) const {
        const TLogbrokerReadMessage* lbMsg = dynamic_cast<const TLogbrokerReadMessage*>(&message);
        if (!lbMsg) {
            TFLEventLog::Error("incorrect message class");
            return IPQClient::TPQResult(false);
        }
        const auto& nativeMsg = lbMsg->GetLogbrokerMessage();
        lbMsg->GetLogbrokerMessage().Commit();
        return IPQClient::TPQResult(GetPartition(nativeMsg.GetPartitionStream()).RequestAck(nativeMsg.GetOffset()).Wait(timeout));
    }

    void TLogbrokerReader::TPartition::AckAck(ui64 offset) {
        auto g = Guard(Lock);
        CommittedOffset = Max(offset, CommittedOffset);
        while (!AcksInFly.empty() && AcksInFly.begin()->first <= offset) {
            AcksInFly.begin()->second.SetValue();
            AcksInFly.erase(AcksInFly.begin());
        }
    }

    NThreading::TFuture<void> TLogbrokerReader::TPartition::RequestAck(ui64 offset) {
        auto g = Guard(Lock);
        if (offset < CommittedOffset) {
            auto p = NThreading::NewPromise();
            p.SetValue();
            return p.GetFuture();
        }
        return AcksInFly.emplace(offset, NThreading::NewPromise()).first->second.GetFuture();
    }

}
