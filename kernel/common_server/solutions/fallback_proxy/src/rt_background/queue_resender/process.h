#pragma once

#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>

#include <kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_reader/process.h>

namespace NCS {
    namespace NFallbackProxy {

        template <typename TMessage>
        class IQueueResenderProcess: public IQueueReaderProcess<TMessage> {
        private:
            using TBase = IQueueReaderProcess<TMessage>;
            CSA_DEFAULT(IQueueResenderProcess, TString, SenderId);
            CSA_DEFAULT(IQueueResenderProcess, TString, CgiMessageId);
            CS_ACCESS(IQueueResenderProcess, TSet<TString>, AcceptedCodes, {"200"});

        public:
            using TMessagesMap = typename TBase::TMessagesMap;
            using TRequestsMap = typename TBase::TRequestsMap;
            class TProcessingContext: public TBase::TReadingContext {
            public:
                TProcessingContext(const typename TBase::TReadingContext& parent)
                    : TBase::TReadingContext(parent) {
                }
            };

        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
                NFrontend::TScheme result = TBase::DoGetScheme(server);
                result.Add<TFSVariants>("sender_id").SetVariants(server.GetAbstractExternalAPINames());
                result.Add<TFSVariants>("accepted_codes").SetMultiSelect(true).InitVariantsAsInteger<HttpCodes>();
                result.Add<TFSString>("cgi_message_id");
                return result;
            }

            virtual NJson::TJsonValue DoSerializeToJson() const override {
                auto result = TBase::DoSerializeToJson();
                TJsonProcessor::Write(result, "sender_id", SenderId);
                TJsonProcessor::Write(result, "cgi_message_id", CgiMessageId);
                TJsonProcessor::WriteContainerArray(result, "accepted_codes", AcceptedCodes);
                return result;
            }

            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::Read(jsonInfo, "sender_id", SenderId, true, false)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "cgi_message_id", CgiMessageId)) {
                    return false;
                }
                if (!TJsonProcessor::ReadContainer(jsonInfo, "accepted_codes", AcceptedCodes, true, false)) {
                    return false;
                }

                return TBase::DoDeserializeFromJson(jsonInfo);
            }

        private:
            virtual bool DoProcessMessages(TRequestsMap requests, typename TBase::TReadingContext& context) const override {
                NExternalAPI::TSender::TPtr sender = context.GetServer().GetSenderPtr(SenderId);
                if (!sender) {
                    TFLEventLog::Error("incorrect sender_id")("sender_id", SenderId);
                    return false;
                }

                auto responses = sender->SendPack(requests);

                for (auto&& [id, response] : responses) {
                    if (AcceptedCodes.contains(::ToString(response.GetReply().Code()))) {
                        context.MutableAcceptedMessages().insert(id);
                    } else {
                        TFLEventLog::Signal("resend_pq_messages")("&code", "rejected_reply_code");
                        context.MutableFailedMessages().insert(id);
                    }
                }
                return true;
            }
        };

        class TResenderProcess: public IQueueResenderProcess<NExternalAPI::TServiceApiHttpDirectRequest> {
        private:
            using TMessage = NExternalAPI::TServiceApiHttpDirectRequest;
            using TBase = IQueueResenderProcess<TMessage>;
            using TRequestsMap = TBase::TRequestsMap;
            static TFactory::TRegistrator<TResenderProcess> Registrator;

            virtual TMaybe<TMessage> ConvertMessage(NCS::IPQMessage::TPtr message) const override;

        public:
            static TString GetTypeName() {
                return "requests_sender";
            }

            virtual TString GetType() const override {
                return GetTypeName();
            }

        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
            virtual NJson::TJsonValue DoSerializeToJson() const override;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        };

    }
}
