#include "controller_command.h"

#include <kernel/common_server/service_monitor/server/server.h>

namespace {
    class TControllerCommandRequest : public NExternalAPI::IHttpRequestWithJsonReport {
    public:
        TControllerCommandRequest(const TCgiParameters& cgi)
            : Cgi(cgi.Print())
        {}

        bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetUri("/")
                .AddHeader("accept", "application/json")
                .AddCgiData(Cgi);
            return true;
        }

        class TResponse : public TJsonResponse {
            CS_ACCESS(TResponse, NJson::TJsonValue, Content, NJson::JSON_NULL);
        protected:
            bool DoParseJsonReply(const NJson::TJsonValue& json) override {
                return DoParseJsonError(json);
            }

            bool DoParseJsonError(const NJson::TJsonValue& json) override {
                Content = json;
                return true;
            }
        };
    private:
        TString Cgi;
    };
}


namespace NServiceMonitor {
    TControllerCommandHandler::TControllerCommandHandler(
        const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TBase(config, context, authModule, server)
    {
    }

    void TControllerCommandHandler::ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) {
        Y_UNUSED(authInfo);

        const auto& server = GetServerAs<NServiceMonitor::TServer>();
        const TString& host = Context->GetCgiParameters().Get("pod_host");
        if (!host) {
            TFLEventLog::Error("host not set");
            g.AddReportElement("error", "host not set");
            g.SetCode(HTTP_BAD_REQUEST);
            return;
        }
        const TString& portStr = Context->GetCgiParameters().Get("controller_port");
        ui16 port = 0;
        if (portStr && !TryFromString(portStr, port)) {
            TFLEventLog::Error("invalid port")("port", portStr);
            g.AddReportElement("error", "host not set");
            g.SetCode(HTTP_BAD_REQUEST);
            return;
        }
        auto sender = server.GetServerInfoStorage().GetSender(host, port);
        auto response = sender->SendRequest<TControllerCommandRequest>(Context->GetCgiParameters());
        auto content = response.GetContent();
        g.AddReportElement("result", std::move(content));
        g.SetCode(response.GetCode());
    }

    TString TControllerCommandHandler::GetTypeName() {
        return "controller_command";
    }

}

