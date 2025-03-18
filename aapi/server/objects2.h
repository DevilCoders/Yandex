#pragma once

#include "event.h"
#include "server.h"

#include <util/generic/queue.h>
#include <util/system/mutex.h>

namespace NAapi {

class TVcsServer::TObject2Request : public TThrRefBase {

    using TThisRef = TIntrusivePtr<TObject2Request>;

    class TRequest : public IQueueEvent {
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

    class TRead : public IQueueEvent {
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

    class TWrite : public IQueueEvent {
    public:
        TWrite(const TThisRef& p)
            : P_(p)
        {
        }

        bool Execute(bool ok) override {
            if (ok) {
                P_->OnWrited();
            } else {
                P_->OnDone();
            }
            return false;
        }

        TThisRef P_;
    };

    class TFinish : public IQueueEvent {
    public:
        TFinish(const TThisRef& p)
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
    TObject2Request(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
        : Server_(server)
        , Service_(service)
        , CQ_(cq)
        , Stream_(&Ctx_)
        , Writing_(false)
        , Finish_(false)
    {
        Service_->RequestObjects2(&Ctx_, &Stream_, CQ_, CQ_, new TRequest(this));
    }

    ~TObject2Request() {
        Server_->Counters_.NSessionObjects2.Dec();
    }

private:
    void OnConnected() {
        Server_->Counters_.NRequestObjects2.Inc();
        Server_->Counters_.NSessionObjects2.Inc();
        Server_->WaitObjects2();

        auto g(Guard(Lock_));
        WaitForNextRequestNoLock();
    }

    void OnReaded() {
        auto g(Guard(Lock_));

        if (Finish_) {
            return;
        }

        bool read = Requests_.empty();

        Requests_.push_back(std::move(Request_));
        WaitForNextRequestNoLock();

        if (read) {
            ScheduleGetObjectNoLock();
        }
    }

    void OnWrited() {
        auto g(Guard(Lock_));
        Responses_.pop();
        if (Responses_) {
            Writing_ = true;
            if (Responses_.front().second.error_code() != grpc::StatusCode::OK) {
                Finish_ = true;
                Stream_.Finish(Responses_.front().second, new TFinish(this));
            } else {
                Stream_.Write(Responses_.front().first, new TWrite(this));
            }
        } else {
            if (Finish_) {
                Stream_.Finish(grpc::Status::OK, new TFinish(this));
            }
            Writing_ = false;
        }
    }

    void OnDone() {
        auto g(Guard(Lock_));
        Finish_ = true;
    }

private:
    void ScheduleGetObjectNoLock() {
        const TString& hash = Requests_.front()->GetHash();

        if (Finish_) {
            return;
        }

        TThisRef ref(this);
        /// Необходимо следить за тем, чтобы одновременно выплнялось
        /// не более одного ScheduleGetObject, иначе ответы могут быть переупорядочены,
        /// так как ScheduleGetObject может выполняться в разных потоках.
        Server_->ScheduleGetObject(hash, [this, ref] (const TString& hash, const TString& blob, const grpc::Status& status)
            {
                auto g(Guard(Lock_));
                Responses_.emplace(TObject(), status);
                Responses_.back().first.SetHash(hash);
                Responses_.back().first.SetBlob(blob);

                if (!Writing_) {
                    Writing_ = true;
                    if (Responses_.front().second.error_code() != grpc::StatusCode::OK) {
                        Finish_ = true;
                        Stream_.Finish(Responses_.front().second, new TFinish(this));
                    } else {
                        Stream_.Write(Responses_.front().first, new TWrite(this));
                    }
                }

                Requests_.pop_front();
                if (Requests_) {
                    ScheduleGetObjectNoLock();
                }

                // Ссылка на объект TObject2Request захватывается специально,
                // чтобы объект не был удалён в случае проблем с grpc-соединением.
                Y_UNUSED(ref);
            }
        );
    }

    void WaitForNextRequestNoLock() {
        Request_.Reset(new ::NAapi::THash);
        Stream_.Read(Request_.Get(), new TRead(this));
    }

private:
    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;
    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;
    grpc::ServerAsyncReaderWriter<TObject, ::NAapi::THash>
            Stream_;

    TMutex Lock_;
    TDeque<THolder<::NAapi::THash>> Requests_;
    THolder<::NAapi::THash> Request_;
    TQueue<std::pair<TObject, grpc::Status>> Responses_;
    bool Writing_;
    bool Finish_;
};

} // namespace NAapi
