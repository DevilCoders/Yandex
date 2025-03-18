#pragma once

#include "event.h"
#include "server.h"

namespace NAapi {

class TVcsServer::TSvnHeadRequest : public IQueueEvent {
public:
    TSvnHeadRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq);

    bool Execute(bool ok) override;

    void DestroyRequest() override;

private:
    bool RequestDone(bool ok);

    bool ReplyDone(bool);

private:
    using TNextStateFunc = bool(TSvnHeadRequest::*)(bool);

    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;
    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;

    TNextStateFunc StateFunc_;
    grpc::ServerAsyncResponseWriter<TRevision> Writer_;
    TEmpty Request_;
};

} // namespace NAapi
