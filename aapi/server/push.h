#pragma once

#include "event.h"
#include "server.h"


namespace NAapi {

class TVcsServer::TPushRequest: public TThrRefBase {

    using TThisRef = TIntrusivePtr<TPushRequest>;

    class TRequest: public IQueueEvent {
    public:
        TRequest(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool ok) override {
            if (ok) {
                P_->OnConnected();
            } else {
                P_->OnDone();
            }
            return false;
        }

        TThisRef P_;
    };

    class TRead: public IQueueEvent {
    public:
        TRead(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool ok) override {
            if (ok) {
                P_->OnReaded();
            } else {
                P_->OnDone();
            }
            return false;
        }

        TThisRef P_;
    };

    class TFinish: public IQueueEvent {
    public:
        TFinish(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool) override {
            P_->OnFinished();
            return false;
        }

        TThisRef P_;
    };

public:
    TPushRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
        : Server_(server)
        , Service_(service)
        , CQ_(cq)
        , Reader_(&Ctx_)
        , FirstRead_(true)
        , Count_(0)
    {
        Service_->RequestPush(&Ctx_, &Reader_, CQ_, CQ_, new TRequest(this));
    }

    void OnConnected() {
        Server_->Counters_.NRequestPush.Inc();
        Server_->WaitPush();

        Trace(TPushStarted());

        auto g(Guard(Lock_));
        WaitNextReadNoLock();
    }

    void OnReaded() {
        auto g(Guard(Lock_));

        if (FirstRead_) {
            Meta_ = Objects_->GetData(0);
            FirstRead_ = false;
        } else {
            for (size_t i = 0; i < Objects_->DataSize(); i += 2) {
                ++Count_;
                Server_->Put(Objects_->GetData(i), Objects_->GetData(i + 1));
            }
        }

        WaitNextReadNoLock();
    }

    void OnDone() {
        auto g(Guard(Lock_));
        Reader_.Finish(TEmpty(), grpc::Status::OK, new TFinish(this));
    }

    void OnFinished() {
        auto g(Guard(Lock_));
        Trace(TPushFinished(Count_, Meta_));
    }

private:
    void WaitNextReadNoLock() {
        Objects_ = MakeHolder<TObjects>();
        Reader_.Read(Objects_.Get(), new TRead(this));
    }

private:
    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;

    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;
    grpc::ServerAsyncReader<TEmpty, TObjects> Reader_;

    TMutex Lock_;
    THolder<TObjects> Objects_;
    bool FirstRead_;
    TString Meta_;
    ui64 Count_;
};

}
