#include "fixed_response_server.h"

void TFixedResponseServer::TServerDeleter::Destroy(TFixedResponseServer* server) {
    server->Stop();
    delete server;
}

TFixedResponseServer::TFixedResponseServer(
        const TOptions& options, const TVector<TFixedResponseServer::TResponse>& responses)
    : NUtil::TAbstractNehServer(options)
    , Responses(responses)
{
}

TFixedResponseServer::TPtr TFixedResponseServer::BuildAndRun(ui16 port,
                                                             const TVector<TResponse>& responses) {
    THttpServerOptions httpOptions(port);
    httpOptions.SetThreads(1);
    NUtil::TAbstractNehServer::TOptions options(httpOptions, "http");
    TPtr server(new TFixedResponseServer(options, responses));
    server->Start();
    return server;
}

size_t TFixedResponseServer::GetCallsCount() const {
    return CallsCount;
}

TFixedResponseServer::TCallbackReply::TCallbackReply(
        NNeh::IRequestRef req, const TFixedResponseServer::TResponse& response)
    : ReqRef(req)
    , Response(response)
{
}

void TFixedResponseServer::TCallbackReply::Process(void* /*threadSpecificResource*/) {
    if (Response.Code == 200) {
        NNeh::TDataSaver reply;
        reply << Response.Reply;
        ReqRef->SendReply(reply);
    } else {
        THashMap<ui16, NNeh::IRequest::TResponseError> errCodes = {
                std::make_pair(400, NNeh::IRequest::BadRequest),
                std::make_pair(404, NNeh::IRequest::NotExistService),
                std::make_pair(500, NNeh::IRequest::InternalError),
        };
        ReqRef->SendError(errCodes[Response.Code]);
    }
}
