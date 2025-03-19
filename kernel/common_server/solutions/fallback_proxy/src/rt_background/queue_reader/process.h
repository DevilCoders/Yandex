#pragma once

#include <kernel/common_server/solutions/fallback_proxy/src/filters/filter.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {
    namespace NFallbackProxy {

        template <typename TMessage>
        class IQueueReaderProcess: public IRTRegularBackgroundProcess {
        private:
            using TBase = IRTRegularBackgroundProcess;
            CSA_DEFAULT(IQueueReaderProcess, TPQMessagesFilterContainer, Filter);
            CSA_DEFAULT(IQueueReaderProcess, TString, QueueId);
            CS_ACCESS(IQueueReaderProcess, ui32, ReadPack, 1000);
            CS_ACCESS(IQueueReaderProcess, bool, DropOnCannotParse, true);
            CS_ACCESS(IQueueReaderProcess, bool, DropOnCannotProcess, true);

        public:
            using TMessagesMap = TMap<TString, IPQMessage::TPtr>;
            using TRequestsMap = TMap<TString, TMessage>;

            class TReadingContext: public TExecutionContext {
                CSA_DEFAULT(TReadingContext, TSet<TString>, AcceptedMessages);
                CSA_DEFAULT(TReadingContext, TSet<TString>, FailedMessages);

            public:
                TReadingContext(const TExecutionContext& parent)
                    : TExecutionContext(parent)
                {
                }
            };

        protected:
            virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> /* state */, const TExecutionContext& context) const override {
                auto& server = context.GetServer();
                IPQClient::TPtr pqClient = server.GetQueuePtr(QueueId);
                if (!pqClient) {
                    TFLEventLog::Error("incorrect queue_id")("queue_id", QueueId);
                    return nullptr;
                }

                TVector<IPQMessage::TPtr> readMessages;
                if (!pqClient->ReadMessages(readMessages, nullptr, ReadPack)) {
                    TFLEventLog::Error("cannot read messages pack")("queue_id", QueueId);
                    return nullptr;
                }

                TVector<IPQMessage::TPtr> filteredMessages;
                if (!Filter) {
                    filteredMessages = std::move(readMessages);
                } else {
                    for (auto message : readMessages) {
                        if (Filter->Accept(message)) {
                            filteredMessages.push_back(message);
                        } else {
                            RemoveMessageFromQueue(pqClient, message, "filtered_out");
                        }
                    }
                }

                TRequestsMap convertedMessages;
                TMessagesMap messageIdToPtr;
                for (auto message : filteredMessages) {
                    TMaybe<TMessage> convertedMessage = ConvertMessage(message);
                    if (convertedMessage) {
                        messageIdToPtr.emplace(message->GetMessageId(), message);
                        convertedMessages.emplace(message->GetMessageId(), std::move(*convertedMessage));
                    } else {
                        TFLEventLog::Signal("read_pq_messages")("&code", "cannot_convert");
                        if (GetDropOnCannotParse()) {
                            RemoveMessageFromQueue(pqClient, message, "not_converted");
                        }
                        continue;
                    }
                }

                TReadingContext localContext(context);
                if (!ProcessMessages(std::move(convertedMessages), localContext)) {
                    TFLEventLog::Error("error occured while processing messages");
                    return nullptr;
                }

                RemoveMessagesFromQueue(pqClient, localContext.GetAcceptedMessages(), messageIdToPtr, "message_with_accepted_reply_code");

                if (DropOnCannotProcess) {
                    TFLEventLog::Signal("removing messages failed at processing");
                    RemoveMessagesFromQueue(pqClient, localContext.GetFailedMessages(), messageIdToPtr, "message_with_rejected_reply_code");
                }

                return new IRTBackgroundProcessState();
            }

            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
                NFrontend::TScheme result = TBase::DoGetScheme(server);
                result.Add<TFSVariants>("queue_id").SetVariants(server.GetQueueNames());
                result.Add<TFSStructure>("filter").SetStructure(TPQMessagesFilterContainer::GetScheme(server), /* required = */ false);
                result.Add<TFSNumeric>("read_pack").SetDefault(1000);
                result.Add<TFSBoolean>("drop_on_cannot_parse").SetDefault(false);
                result.Add<TFSBoolean>("drop_on_cannot_process").SetDefault(false);
                return result;
            }

            virtual NJson::TJsonValue DoSerializeToJson() const override {
                auto result = TBase::DoSerializeToJson();
                TJsonProcessor::Write(result, "queue_id", QueueId);
                TJsonProcessor::Write(result, "read_pack", ReadPack);
                TJsonProcessor::Write(result, "drop_on_cannot_parse", DropOnCannotParse);
                TJsonProcessor::Write(result, "drop_on_cannot_process", DropOnCannotProcess);
                TJsonProcessor::WriteObject(result, "filter", Filter);
                return result;
            }

            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::Read(jsonInfo, "queue_id", QueueId, true, false)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "drop_on_cannot_parse", DropOnCannotParse)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "drop_on_cannot_process", DropOnCannotProcess)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "read_pack", ReadPack)) {
                    return false;
                }
                if (!TJsonProcessor::ReadObject(jsonInfo, "filter", Filter)) {
                    return false;
                }

                return TBase::DoDeserializeFromJson(jsonInfo);
            }

        private:
            bool ProcessMessages(TRequestsMap messages, TReadingContext& context) const {
                return DoProcessMessages(std::move(messages), context);
            }

            void RemoveMessageFromQueue(const IPQClient::TPtr pqClient, const NCS::IPQMessage::TPtr message, const TString& signalCode) const {
                const bool success = pqClient->AckMessage(message).IsSuccess();
                TFLEventLog::Signal("resend_pq_messages")("&code", (success ? "managed_to_ack_" : "failed_to_ack_") + signalCode);
            }

            void RemoveMessagesFromQueue(const IPQClient::TPtr pqClient, const TSet<TString>& messageIds, const TMessagesMap& messageIdToPtr, const TString& signalCode) const {
                for (auto& messageId : messageIds) {
                    auto it = messageIdToPtr.find(messageId);
                    CHECK_WITH_LOG(it != messageIdToPtr.end()) << "Unexpected message id";
                    RemoveMessageFromQueue(pqClient, it->second, signalCode);
                }
            }

            virtual bool DoProcessMessages(TRequestsMap messages, TReadingContext& context) const = 0;
            virtual TMaybe<TMessage> ConvertMessage(NCS::IPQMessage::TPtr message) const = 0;

        public:
            virtual bool IsSimultaneousProcess() const override {
                return true;
            }
        };
    }
}
