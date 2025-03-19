#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/library/tvm_services/abstract/request/abstract.h>

#include <library/cpp/logger/global/global.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>
#include <util/system/compiler.h>

namespace NCS {

    class IPQMessage {
    public:
        using TPtr = TAtomicSharedPtr<IPQMessage>;
        virtual ~IPQMessage() = default;
        virtual TBlob GetContent() const = 0;
        virtual const TString& GetMessageId() const = 0;
    };

    class TPQMessageSimple: public IPQMessage {
    private:
        TBlob Content;
        TString MessageId = TGUID::CreateTimebased().AsUuidString();
    public:
        TPQMessageSimple() = default;
        TPQMessageSimple(TBlob&& content, const TString& messageId)
            : Content(std::move(content))
            , MessageId(messageId) {

        }
        TPQMessageSimple& SetContent(const TBlob& data) {
            Content = data;
            return *this;
        }
        TPQMessageSimple& SetContent(TBlob&& data) {
            Content = std::move(data);
            return *this;
        }
        virtual TBlob GetContent() const override {
            return TBlob::NoCopy(Content.data(), Content.Length());
        }
        virtual const TString& GetMessageId() const override {
            return MessageId;
        }
    };

    class IPQClient: public IAutoActualization {
    private:
        using TBase = IAutoActualization;
    public:
        using TPtr = TAtomicSharedPtr<IPQClient>;
        class TPQResult {
            CSA_DEFAULT(TPQResult, NExternalAPI::TRequestSessionContainer, RequestSession);
            CSA_DEFAULT(TPQResult, TString, Id);
            CSA_FLAG(TPQResult, Success, false);
        public:
            TPQResult() = default;
            explicit TPQResult(const bool result)
                : SuccessFlag(result) {
            }

            bool operator !() const {
                return !IsSuccess();
            }
        };
    private:
        CSA_DEFAULT(IPQClient, TString, ClientId);
    protected:
        virtual bool DoStartImpl() = 0;
        virtual bool DoStopImpl() = 0;
        virtual bool DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const = 0;
        virtual TPQResult DoWriteMessage(const IPQMessage& result) const = 0;
        virtual TPQResult DoAckMessage(const IPQMessage& message) const = 0;
        virtual bool DoFlushWritten() const = 0;
        virtual bool IsReadable() const = 0;
        virtual bool IsWritable() const = 0;

        virtual bool DoStart() override final;
        virtual bool DoStop() override final;
        virtual bool Refresh() override {
            return true;
        }
    public:
        Y_WARN_UNUSED_RESULT bool ReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages = nullptr, const size_t maxSize = Max<size_t>(), const TDuration timeout = Max<TDuration>(), const bool wait = false) const noexcept;
        Y_WARN_UNUSED_RESULT TPQResult AckMessage(IPQMessage::TPtr message) const noexcept;
        IPQMessage::TPtr ReadMessage(ui32* failedMessages = nullptr, const TDuration timeout = TDuration::Max(), const bool wait = false) const noexcept;

        Y_WARN_UNUSED_RESULT virtual bool FlushWritten() const noexcept;
        TPQResult WriteMessage(IPQMessage::TPtr message) const noexcept;
        template <class TRequest>
        TPQResult SendRequest(const TRequest& r) const {
            auto gLogging = TFLRecords::StartContext()("&pq_client_id", ClientId)("&message_type", r.GetMessageType());
            NCS::NLogging::TDefaultLogsAccumulator accumulator;
            IPQMessage::TPtr qRequest = r.BuildMessage();
            if (!qRequest) {
                TFLEventLog::Signal("pq_send_request")("&code", "cannot_construct_request");
                TPQResult resultFail = TPQResult(false);
                resultFail.SetRequestSession(NExternalAPI::THTTPRequestSession::BuildFailed(accumulator));
                return resultFail;
            }
            return WriteMessage(qRequest);
        }
    public:
        IPQClient(const TString& clientId)
            : TBase(clientId)
            , ClientId(clientId) {

        }
        virtual ~IPQClient() = default;
    };

    class IPQRandomAccess: public IPQClient {
    private:
        using TBase = IPQClient;
    protected:
        virtual bool DoRestoreMessage(const TString& messageId, IPQMessage::TPtr& result) const = 0;
    public:
        using TBase::TBase;
        bool RestoreMessage(const TString& messageId, IPQMessage::TPtr& result) const noexcept {
            auto gLogging = TFLRecords::StartContext().Method("IPQRandomAccess::RestoreMessage")("message_id", messageId);
            try {
                return DoRestoreMessage(messageId, result);
            } catch (...) {
                TFLEventLog::Error(CurrentExceptionMessage());
                return false;
            }
        }
    };
}
