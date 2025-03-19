#include "client.h"
#include "server.h"
#include "replier.h"

void TFrontendClientRequest::MakeErrorPage(IReplyContext::TPtr context, ui32 code, const TString& error) const {
    Y_ASSERT(!!context);
    TJsonReport report(context, context->GetUri());
    report.AddReportElement("error", error);
    report.Finish(code);
}

IHttpReplier::TPtr TFrontendClientRequest::DoSelectHandler(IReplyContext::TPtr context) {
    return MakeHolder<TReplier>(context, Config, Server);
}

TFrontendClientRequest::TFrontendClientRequest(const TBaseServerConfig* config, const IBaseServer* server)
    : Config(config)
    , Server(server) {

}
