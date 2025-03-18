#pragma once

#include "event.h"
#include "server.h"

namespace NAapi {

class TVcsServer::THgIdRequest : public IQueueEvent {
public:
    THgIdRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq);

    bool Execute(bool ok) override;

    void DestroyRequest() override;

private:
    bool RequestDone(bool ok);

    bool ReplyDone(bool);

private:
    using TNextStateFunc = bool(THgIdRequest::*)(bool);

    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;
    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;

    TNextStateFunc StateFunc_;
    grpc::ServerAsyncResponseWriter<THash> Writer_;
    TName Request_;
};

} // namespace NAapi
