#pragma once

#include "event.h"
#include "server.h"


namespace NAapi {

class TVcsServer::TNotifyNewObjectsRequest: public TThrRefBase {

    using TThisRef = TIntrusivePtr<TNotifyNewObjectsRequest>;

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
    TNotifyNewObjectsRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
        : Server_(server)
        , Service_(service)
        , CQ_(cq)
        , Reader_(&Ctx_)
        , FirstRead_(true)
        , Count_(0)
    {
        Service_->RequestNotifyNewObjects(&Ctx_, &Reader_, CQ_, CQ_, new TRequest(this));
    }

    void OnConnected() {
        Server_->Counters_.NRequestsNotifyNewObjects.Inc();
        Server_->WaitNotifyNewObjects();

        Trace(TNotifyStarted());

        auto g(Guard(Lock_));
        WaitNextReadNoLock();
    }

    void OnReaded() {
        auto g(Guard(Lock_));

        if (FirstRead_) {
            Meta_ = Hashes_->GetHashes(0);
            FirstRead_ = false;
        } else {
            for (const TString& hash: Hashes_->GetHashes()) {
                ++Count_;
                Server_->ScheduleGetObject(hash, [](const TString&, const TString&, const grpc::Status&){});
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
        Trace(TNotifyFinished(Count_, Meta_));
    }

private:
    void WaitNextReadNoLock() {
        Hashes_ = MakeHolder<THashes>();
        Reader_.Read(Hashes_.Get(), new TRead(this));
    }

private:
    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;

    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;
    grpc::ServerAsyncReader<TEmpty, THashes> Reader_;

    TMutex Lock_;
    THolder<THashes> Hashes_;
    bool FirstRead_;
    TString Meta_;
    ui64 Count_;
};

}
