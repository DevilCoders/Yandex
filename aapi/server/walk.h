#pragma once

#include "event.h"
#include "server.h"

#include <util/generic/queue.h>
#include <util/string/hex.h>
#include <util/system/mutex.h>

namespace NAapi {

class TVcsServer::TWalkRequest : public IWalkReceiver
{
    using TThisRef = TIntrusivePtr<TWalkRequest>;

    class TRequestDone : public IQueueEvent {
    public:
        TRequestDone(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool ok) override {
            if (ok) {
                P_->OnRequest();
            } else {
                P_->OnDone();
            }
            return false;
        }

        TThisRef P_;
    };

    class TWriteDone : public IQueueEvent {
    public:
        TWriteDone(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool ok) override {
            if (ok) {
                P_->OnWriteDone();
            } else {
                P_->OnDone();
            }
            return false;
        }

        TThisRef P_;
    };

    class TFinishDone : public IQueueEvent {
    public:
        TFinishDone(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool) override {
            P_->OnDone();
            return false;
        }

        TThisRef P_;
    };

public:
    TWalkRequest(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
        : Server_(server)
        , Service_(service)
        , CQ_(cq)
        , FinishStatus_(grpc::Status::OK)
        , Writing_(false)
        , Finish_(false)
        , Stream_(&Ctx_)
    {
        Service_->RequestWalk(&Ctx_, &Request_, &Stream_, CQ_, CQ_, new TRequestDone(this));
    }

    ~TWalkRequest() {
        Server_->Counters_.NSessionWalk.Dec();
    }

    bool Push(const TDirectories& dirs) override {
        auto g(Guard(Lock_));
        if (Finish_) {
            return false;
        }
        Dirs_.push(dirs);
        if (!Writing_) {
            Writing_ = true;
            Stream_.Write(Dirs_.front(), new TWriteDone(this));
        }
        return true;
    }

    void Finish(const grpc::Status& status) override {
        auto g(Guard(Lock_));
        Finish_ = true;
        FinishStatus_ = status;
        if (Dirs_.empty()) {
            Stream_.Finish(FinishStatus_, new TFinishDone(this));
        }
    }

private:
    void OnRequest() {
        Server_->Counters_.NRequestWalk.Inc();
        Server_->Counters_.NSessionWalk.Inc();
        Server_->WaitWalk();

        Server_->AsyncWalk(Request_.GetHash(), this);
    }

    void OnWriteDone() {
        auto g(Guard(Lock_));
        Dirs_.pop();
        if (Dirs_) {
            Writing_ = true;
            Stream_.Write(Dirs_.front(), new TWriteDone(this));
        } else {
            if (Finish_) {
                Stream_.Finish(FinishStatus_, new TFinishDone(this));
            }
            Writing_ = false;
        }
    }

    void OnDone() {
        auto g(Guard(Lock_));
        Finish_ = true;
    }

private:
    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;
    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;
    grpc::Status FinishStatus_;

    TMutex Lock_;
    TQueue<TDirectories> Dirs_;
    bool Writing_;
    bool Finish_;

    ::NAapi::THash Request_;
    grpc::ServerAsyncWriter<TDirectories> Stream_;
};

} // namespace NAapi
