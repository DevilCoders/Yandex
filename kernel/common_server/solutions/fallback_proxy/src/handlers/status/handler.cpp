#include "handler.h"
#include <kernel/common_server/solutions/fallback_proxy/src/pq_message/message.h>
#include <util/string/subst.h>

namespace NCS {
    namespace NFallbackProxy {
        class TStatusProxyAgent: public NForwardProxy::TDirectConstructor {
        private:
            using TBase = NForwardProxy::TDirectConstructor;
            const IBaseServer& Server;
            const TStatusProcessorConfig& Config;
            TMaybe<NJson::TJsonValue> JsonData;
            TString BuildMessageId() const {
                if (!!Config.GetInputPathForMessageId()) {
                    if (!JsonData) {
                        TFLEventLog::Error("non json incoming data");
                        return "";
                    }
                    const NJson::TJsonValue* jsonValue = JsonData->GetValueByPath(Config.GetInputPathForMessageId());
                    if (!jsonValue) {
                        TFLEventLog::Error("incorrect message id path")("path", Config.GetInputPathForMessageId())("raw_data", *JsonData);
                        return "";
                    } else {
                        return jsonValue->GetStringRobust();
                    }
                } else {
                    return "";
                }
            }
            bool DoFallback(TJsonReport::TGuard& g, const TString& reason, const TString& eventInfo) const {
                auto gLogging = TFLRecords::StartContext().Method("TStatusProxyAgent::DoFallback");
                TFLEventLog::Notice("fallback happened")("reason", reason)("event", eventInfo);
                const TString messageId = BuildMessageId();
                if (!messageId) {
                    TFLEventLog::Error("cannot build message id");
                    return false;
                }
                if (!!Config.GetPersistentQueueId()) {
                    auto queue = Server.GetQueuePtr(Config.GetPersistentQueueId());
                    if (!queue) {
                        TFLEventLog::Error("incorrect persistent queue for proxy")("queue_id", Config.GetPersistentQueueId());
                        return false;
                    }
                    TAtomicSharedPtr<IPQRandomAccess> raQueue = DynamicPointerCast<IPQRandomAccess>(queue);
                    if (!raQueue) {
                        TFLEventLog::Error("incorrect queue for message id providing")("queue_id", Config.GetPersistentQueueId());
                        return false;
                    }
                    IPQMessage::TPtr message;
                    if (!raQueue->RestoreMessage(messageId, message)) {
                        TFLEventLog::Error("cannot check restored message");
                        return false;
                    } else if (message) {
                        TFLEventLog::Notice("message detected in queue");
                    } else {
                        TFLEventLog::Error("unknown message")("message_id", messageId);
                        return false;
                    }
                }
                g.SetExternalReportString(SubstGlobalCopy<TString, TString>(Config.GetReplyTemplate(), "$MESSAGE_ID", messageId), false);
                g.SetCode(Config.GetReplyCode());
                return true;
            }

        protected:
            virtual bool DoBuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const override {
                if (Config.NeedFallback(originalResponse.Code()) && Config.GetReplyCode()) {
                    if (!DoFallback(g, "original_code: " + ::ToString(originalResponse.Code()), "original_server_error")) {
                        g.SetCode(HTTP_SERVICE_UNAVAILABLE);
                        return false;
                    } else {
                        return true;
                    }
                } else {
                    return TBase::DoBuildReport(request, originalResponse, g);
                }
            }

            virtual bool DoBuildReportOnException(const NNeh::THttpRequest& request, const TString& exceptionMessage, TJsonReport::TGuard& g) const override {
                if (Config.GetReplyCode()) {
                    if (!DoFallback(g, exceptionMessage, "exception")) {
                        g.SetCode(HTTP_SERVICE_UNAVAILABLE);
                        return false;
                    } else {
                        return true;
                    }
                } else {
                    return TBase::DoBuildReportOnException(request, exceptionMessage, g);
                }
            }
            virtual bool DoBuildReportNoReply(const NNeh::THttpRequest& request, TJsonReport::TGuard& g) const override {
                if (Config.GetReplyCode()) {
                    if (!DoFallback(g, "", "original_server_no_reply")) {
                        g.SetCode(HTTP_SERVICE_UNAVAILABLE);
                        return false;
                    } else {
                        return true;
                    }
                } else {
                    return TBase::DoBuildReportNoReply(request, g);
                }
            }

        public:
            TStatusProxyAgent(const IBaseServer& server, const TStatusProcessorConfig& config, const NJson::TJsonValue* jsonData)
                : Server(server)
                , Config(config)
            {
                if (jsonData) {
                    JsonData = *jsonData;
                }
            }
        };

        NCS::NForwardProxy::TReportConstructorContainer TStatusProcessor::BuildReportConstructor() const {
            return new TStatusProxyAgent(GetServer(), Config, HasJsonData() ? &GetJsonData() : nullptr);
        }

    }
}
