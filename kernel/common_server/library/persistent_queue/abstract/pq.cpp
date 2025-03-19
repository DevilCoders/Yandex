#include "pq.h"
#include <library/cpp/logger/global/global.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <kernel/common_server/library/logging/events.h>

namespace NCS {

    bool IPQClient::DoStart() {
        auto gLogging = TFLRecords::StartContext()("&pq_client_id", ClientId).Method("Start");
        if (!DoStartImpl()) {
            return false;
        }
        if (!TBase::DoStart()) {
            return false;
        }
        return true;
    }

    bool IPQClient::DoStop() {
        auto gLogging = TFLRecords::StartContext()("&pq_client_id", ClientId).Method("Stop");
        if (!TBase::DoStop()) {
            return false;
        }
        if (!DoStopImpl()) {
            return false;
        }
        return true;
    }

    bool IPQClient::ReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize /*= Max<size_t>()*/, const TDuration timeout /*= Max<TDuration>()*/, const bool wait /*= false*/) const noexcept {
        auto gLogging = TFLRecords::StartContext()("&pq_client_id", ClientId).Method("ReadMessages");
        if (!IsActive()) {
            TFLEventLog::Error("pq not started");
            return false;
        }
        if (!IsReadable()) {
            TFLEventLog::Error("pq does not configured for read");
            return false;
        }
        const auto deadline = timeout.ToDeadLine();
        try {
            do {
                ui32 failedInternal = 0;
                const bool resultFlag = DoReadMessages(result, &failedInternal, maxSize, timeout);
                if (failedInternal) {
                    TFLEventLog::Signal("pq_read", failedInternal)("&code", "failed_messages").Error();
                }
                if (failedMessages) {
                    *failedMessages = failedInternal;
                }
                if (resultFlag) {
                    TFLEventLog::JustSignal("pq_read", result.size())("&code", "success");
                } else {
                    TFLEventLog::Signal("pq_read")("&code", "cannot_read").Error();
                    return false;
                }
            } while (wait && result.size() < maxSize && Now() < deadline);
        } catch (...) {
            TFLEventLog::Signal("pq_read")("&code", "exception")("reason", CurrentExceptionMessage()).Error();
            return false;
        }
        return true;
    }

    IPQClient::TPQResult IPQClient::AckMessage(IPQMessage::TPtr message) const noexcept {
        auto gLogging = TFLRecords::StartContext().SignalId("pq_ack")("&pq_client_id", ClientId).Method("AckMessage");
        NCS::NLogging::TDefaultLogsAccumulator accumulator;
        IPQClient::TPQResult resultFail = IPQClient::TPQResult(false);
        if (!IsActive()) {
            TFLEventLog::Signal("pq_ack")("&code", "fail").Error("pq not started");
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
        if (!IsReadable()) {
            TFLEventLog::Signal("pq_ack")("&code", "fail").Error("pq does not configured for read");
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
        if (!message) {
            TFLEventLog::Signal("pq_ack")("&code", "fail").Error("cannot ack empty message");
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
        try {
            IPQClient::TPQResult result = DoAckMessage(*message);
            if (!!result) {
                TFLEventLog::Signal("pq_ack")("&code", "success")("message_id", message->GetMessageId());
            } else {
                TFLEventLog::Signal("pq_ack")("&code", "fail")("message_id", message->GetMessageId()).Error();
                auto requestSession = result.MutableRequestSession();
                if (!requestSession) {
                    result.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
                } else {
                    requestSession->SetProblemDetails(accumulator);
                }
            }
            return result;
        } catch (...) {
            TFLEventLog::Signal("pq_ack")("&code", "fail")("message_id", message->GetMessageId()).Error(CurrentExceptionMessage());
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
    }

    IPQMessage::TPtr IPQClient::ReadMessage(ui32* failedMessages, const TDuration timeout /*= TDuration::Max()*/, const bool wait /*= false*/) const noexcept {
        TVector<IPQMessage::TPtr> messages;
        if (!ReadMessages(messages, failedMessages, 1, timeout, wait) || messages.empty()) {
            return nullptr;
        }
        CHECK_WITH_LOG(messages.size() < 2);
        return messages.front();
    }

    bool IPQClient::FlushWritten() const noexcept {
        auto gLogging = TFLRecords::StartContext()("&pq_client_id", ClientId).Method("FlushWritten");
        if (!IsActive()) {
            TFLEventLog::Error("pq not started");
            return false;
        }
        if (!IsWritable()) {
            TFLEventLog::Error("pq does not configured for write");
            return false;
        }
        try {
            return DoFlushWritten();
        } catch (...) {
            TFLEventLog::Error(CurrentExceptionMessage());
            return false;
        }
    }

    IPQClient::TPQResult IPQClient::WriteMessage(IPQMessage::TPtr message) const noexcept {
        NCS::NLogging::TDefaultLogsAccumulator accumulator;
        auto gLogging = TFLRecords::StartContext()("&pq_client_id", ClientId).Method("WriteMessage");
        TPQResult resultFail = TPQResult(false);
        if (!IsActive()) {
            TFLEventLog::Error("pq not started");
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
        if (!IsWritable()) {
            TFLEventLog::Error("pq does not configured for write");
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
        if (!message) {
            TFLEventLog::Error("cannot write empty message");
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
        try {
            auto result = DoWriteMessage(*message);
            if (!!result) {
                TFLEventLog::Signal("pq_write")("&code", "success")("message_id", message->GetMessageId());
                result.SetId(message->GetMessageId());
            } else {
                TFLEventLog::Signal("pq_write")("&code", "cannot_write")("message_id", message->GetMessageId()).Error();
                auto requestSession = result.MutableRequestSession();
                if (!requestSession) {
                    result.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
                } else {
                    requestSession->SetProblemDetails(accumulator);
                }
            }
            return result;
        } catch (...) {
            TFLEventLog::Signal("pq_write")("&code", "exception")("reason", CurrentExceptionMessage())("message_id", message->GetMessageId()).Error();
            resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
            return resultFail;
        }
    }
}

