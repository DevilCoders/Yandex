#include "handler.h"
#include <kernel/common_server/solutions/fallback_proxy/src/pq_message/message.h>

namespace NCS {
    namespace NFallbackProxy {
        class TWriterProxyAgent: public NForwardProxy::TDirectConstructor {
        private:
            using TBase = NForwardProxy::TDirectConstructor;
            const IBaseServer& Server;
            const TWriterProcessorConfig& Config;
            TMaybe<NJson::TJsonValue> JsonData;
            TString BuildMessageId() const {
                if (!!Config.GetInputPathForMessageId()) {
                    if (!JsonData) {
                        TFLEventLog::Error("non json report for path usage");
                        return "";
                    }
                    const NJson::TJsonValue* jsonValue = JsonData->GetValueByPath(Config.GetInputPathForMessageId());
                    if (!jsonValue) {
                        TFLEventLog::Error("incorrect path in request for input json path usage")("path", Config.GetInputPathForMessageId())("raw_data", *JsonData);
                        return "";
                    } else {
                        return jsonValue->GetStringRobust();
                    }
                } else {
                    return TGUID::CreateTimebased().AsUuidString();
                }
            }
            bool DoFallback(TJsonReport::TGuard& g, const NNeh::THttpRequest& request, const TString& originalEvent, const TString& originalReason) const {
                auto gLogging = TFLRecords::StartContext().Method("TWriterProxyAgent::DoFallback");
                TFLEventLog::Notice("fallback happened")("reason", originalReason)("event", originalEvent);
                auto queue = Server.GetQueuePtr(Config.GetPersistentQueueId());
                if (!queue) {
                    TFLEventLog::Error("incorrect persistent queue for proxy")("queue_id", Config.GetPersistentQueueId());
                    return false;
                }
                const TString resultMessageId = BuildMessageId();
                if (!resultMessageId) {
                    TFLEventLog::Error("cannot build message id");
                    return false;
                }
                if (Config.GetInputPathForMessageId()) {
                    TAtomicSharedPtr<IPQRandomAccess> raQueue = DynamicPointerCast<IPQRandomAccess>(queue);
                    if (!raQueue) {
                        TFLEventLog::Error("incorrect queue for message id reuse")("queue_id", Config.GetPersistentQueueId());
                        return false;
                    }
                    IPQMessage::TPtr message;
                    if (!raQueue->RestoreMessage(resultMessageId, message)) {
                        TFLEventLog::Error("cannot check restored message");
                        return false;
                    } else if (!!message) {
                        if (Config.GetOutputPathForMessageId()) {
                            g.AddReportElement(Config.GetOutputPathForMessageId(), resultMessageId);
                        }
                        g.SetCode(HTTP_ACCEPTED);
                        return true;
                    }
                }
                if (!queue->WriteMessage(TRequestPQMessage::BuildRequestPQMessage(request, resultMessageId))) {
                    TFLEventLog::Error("cannot store message");
                    return false;
                } else {
                    if (Config.GetOutputPathForMessageId()) {
                        g.AddReportElement(Config.GetOutputPathForMessageId(), resultMessageId);
                    }
                    g.SetCode(HTTP_ACCEPTED);
                    return true;
                }
            }

        protected:
            virtual bool DoBuildReport(const NNeh::THttpRequest& request, const NUtil::THttpReply& originalResponse, TJsonReport::TGuard& g) const override {
                if (Config.GetPersistentQueueId() && IsServerError(originalResponse.Code()) && Config.IsServerErrorsForWriting()) {
                    if (!DoFallback(g, request, "server_error", ::ToString(originalResponse.Code()))) {
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
                if (Config.GetPersistentQueueId()) {
                    if (!DoFallback(g, request, "exception", exceptionMessage)) {
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
                if (Config.GetPersistentQueueId()) {
                    if (!DoFallback(g, request, "no_reply", "")) {
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
            TWriterProxyAgent(const IBaseServer& server, const TWriterProcessorConfig& config, const NJson::TJsonValue* jsonData)
                : Server(server)
                , Config(config)
            {
                if (jsonData) {
                    JsonData = *jsonData;
                }
            }
        };

        NCS::NForwardProxy::TReportConstructorContainer TWriterProcessor::BuildReportConstructor() const {
            return new TWriterProxyAgent(GetServer(), Config, HasJsonData() ? &GetJsonData() : nullptr);
        }

    }
}
