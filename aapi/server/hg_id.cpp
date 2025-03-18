#include "hg_id.h"

namespace NAapi {

TVcsServer::THgIdRequest::THgIdRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : Server_(server)
    , Service_(service)
    , CQ_(cq)
    , StateFunc_(&THgIdRequest::RequestDone)
    , Writer_(&Ctx_)
{
    Service_->RequestHgId(&Ctx_, &Request_, &Writer_, CQ_, CQ_, this);
}

bool TVcsServer::THgIdRequest::Execute(bool ok) {
    return (this->*StateFunc_)(ok);
}

void TVcsServer::THgIdRequest::DestroyRequest() {
    delete this;
}

bool TVcsServer::THgIdRequest::RequestDone(bool ok) {
    if (!ok) {
        return false;
    }

    // Current request has been received,
    // so start waiting for next one.
    Server_->WaitHgId();
    Server_->Counters_.NRequestHgId.Inc();
    Server_->AsyncGetHgId(Request_.GetName())
        .Subscribe([this] (NThreading::TFuture<TString> result) {
            TString h = result.GetValue();
            THash resp;
            resp.SetHash(h);

            StateFunc_ = &THgIdRequest::ReplyDone;

            if (!h) {
                Writer_.Finish(resp, grpc::Status(grpc::StatusCode::INTERNAL, ""), this);
            } else {
                Writer_.Finish(resp, grpc::Status::OK, this);
            }
        });

    return true;
}

bool TVcsServer::THgIdRequest::ReplyDone(bool) {
    return false;
}

} // namespace NAapi
