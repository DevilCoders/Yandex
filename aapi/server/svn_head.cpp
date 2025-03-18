#include "svn_head.h"

namespace NAapi {

TVcsServer::TSvnHeadRequest::TSvnHeadRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : Server_(server)
    , Service_(service)
    , CQ_(cq)
    , StateFunc_(&TSvnHeadRequest::RequestDone)
    , Writer_(&Ctx_)
{
    Service_->RequestSvnHead(&Ctx_, &Request_, &Writer_, CQ_, CQ_, this);
}

bool TVcsServer::TSvnHeadRequest::Execute(bool ok) {
    return (this->*StateFunc_)(ok);
}

void TVcsServer::TSvnHeadRequest::DestroyRequest() {
    delete this;
}

bool TVcsServer::TSvnHeadRequest::RequestDone(bool ok) {
    if (!ok) {
        return false;
    }

    // Current request has been received,
    // so start waiting for next one.
    Server_->WaitSvnHead();
    Server_->Counters_.NRequestSvnHead.Inc();
    Server_->AsyncGetSvnHead()
        .Subscribe([this] (NThreading::TFuture<ui64> result) {
            TRevision resp;
            resp.SetRevision(result.GetValue());
            StateFunc_ = &TSvnHeadRequest::ReplyDone;
            Writer_.Finish(resp, grpc::Status::OK, this);
        });

    return true;
}

bool TVcsServer::TSvnHeadRequest::ReplyDone(bool) {
    return false;
}

} // namespace NAapi
