#include "writer.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {

    TLogbrokerWriter::TLogbrokerWriter(const TLogbrokerClientConfig::TWriteConfig& config)
        : Config(config)
    {
        TThreadPool::TParams params;
        params.SetBlocking(true);
        WriteThread = MakeHolder<TThreadPool>(params);
    }


    bool TLogbrokerWriter::Start(NYdb::NPersQueue::TPersQueueClient& client) {
        auto sessionConfig = Config.BuildSessionConfig();
        NYdb::NPersQueue::TWriteSessionSettings::TEventHandlers handlers;

        handlers.AcksHandler([this](NYdb::NPersQueue::TWriteSessionEvent::TAcksEvent& acks) {
            for (const auto& ack : acks.Acks) {
                if (ack.State != NYdb::NPersQueue::TWriteSessionEvent::TWriteAck::EES_DISCARDED) {
                    auto g = Guard(ContentBufferMutex);
                    while (!ContentBuffer.empty() && ContentBuffer.front().IsEarlerThen(ack.SeqNo)) {
                        ContentBuffer.front().Acked.TrySetValue();
                        ContentBuffer.pop_front();
                    }
                }
            }
        });

        sessionConfig.EventHandlers(handlers);
        if (!(Session = client.CreateWriteSession(sessionConfig))) {
            TFLEventLog::Error("Cannot create write section");
            return false;
        }
        LastWrittenSeqNo = Session->GetInitSeqNo().GetValueSync();
        WriteThread->Start(1, Config.GetMaxInFlight());
        return true;
    }

    bool TLogbrokerWriter::Stop() {
        WriteThread->Stop();
        if (Session && !Session->Close()) {
            TFLEventLog::Error("close Producer failed");
            return false;
        }
        Session.reset();
        return true;
    }

    IPQClient::TPQResult TLogbrokerWriter::WriteMessage(const IPQMessage& msg, const TDuration /*timeout*/) {
        ui64 seqNo = 0;
        if (msg.GetMessageId() && !TryFromString(msg.GetMessageId(), seqNo)) {
            TFLEventLog::Error("TLogbrokerClient supports only integer MessageId, but " + msg.GetMessageId() + " set");
            return IPQClient::TPQResult(false);
        }
        TWriteMessage* toWrite; //Указатель будет жив, так как удалить из буфера данные может только пришедший Ack, а его не может быть на неотправленное сообщение.
        with_lock(ContentBufferMutex) {
            const auto& ctnt = msg.GetContent();
            ContentBuffer.emplace_back(seqNo, ctnt.OwnsData() ? ctnt : TBlob::Copy(ctnt.Data(), ctnt.Length()));
            toWrite = &ContentBuffer.back();
        }
        WriteThread->SafeAddFunc([this, toWrite]() {
            if (!toWrite->SeqNo) {
                toWrite->SeqNo = LastWrittenSeqNo + 1;
            } else if (toWrite->SeqNo <= LastWrittenSeqNo) {
                toWrite->Acked.SetValue();
                return;
            };
            LastWrittenSeqNo = toWrite->SeqNo;
            while (true) {
                if (auto* ready = std::get_if<NYdb::NPersQueue::TWriteSessionEvent::TReadyToAcceptEvent>(Session->GetEvent(true).Get())) {
                    Session->Write(std::move(ready->ContinuationToken), toWrite->Content.AsStringBuf(), toWrite->SeqNo);
                    return;
                }
            }
        });
        return IPQClient::TPQResult(true);
    }

    bool TLogbrokerWriter::FlushWritten(const TDuration timeout) {
        NThreading::TFuture<void> f;
        const auto deadline = timeout.ToDeadLine();
        with_lock(ContentBufferMutex) {
            if (!ContentBuffer.empty()) {
                f = ContentBuffer.back().Acked.GetFuture();
            } else {
                return true;
            }
        }
        return f.Wait(deadline);
    }

    TLogbrokerWriter::TWriteMessage::TWriteMessage(const ui64 seqNo, const TBlob& content)
        : SeqNo(seqNo)
        , Content(content)
        , Acked(NThreading::NewPromise())
    {}

    bool TLogbrokerWriter::TWriteMessage::IsEarlerThen(const ui64 seqNo) const {
        if (!SeqNo) {
            return false;   // Not sent yet
        }
        if (Acked.HasValue()) {
            return true;    // Resend
        }
        return SeqNo <= seqNo;
    }

}
