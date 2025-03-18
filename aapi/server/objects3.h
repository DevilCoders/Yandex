#pragma once

#include "event.h"
#include "server.h"

namespace NAapi {

class TVcsServer::TObject3Request : public TThrRefBase {

    using TThisRef = TIntrusivePtr<TObject3Request>;

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
            P_->Server_->Counters_.NObjects3ReadObjects.Inc();
        }

        ~TRead() {
            P_->Server_->Counters_.NObjects3ReadObjects.Dec();
        }

        bool Execute(bool ok) override {
            if (ok) {
                P_->OnReaded();
            } else {
                P_->OnReadsDone();
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
    TObject3Request(TVcsServer* server, NVcs::Vcs::AsyncService* service, grpc::ServerCompletionQueue* cq)
        : Server_(server)
        , Service_(service)
        , CQ_(cq)
        , Stream_(&Ctx_)
        , CurrentResponseSize_(0)
        , HashesInProgress_(0)
        , Writing_(false)
        , Reading_(false)
    {
        Service_->RequestObjects3(&Ctx_, &Stream_, CQ_, CQ_, new TRequest(this));
    }

    ~TObject3Request() {
        Server_->Counters_.NSessionObjects3.Dec();
    }

private:
    void OnConnected() {
        Server_->Counters_.NRequestObjects3.Inc();
        Server_->Counters_.NSessionObjects3.Inc();
        Server_->WaitObjects3();

        auto g(Guard(Lock_));
        Reading_ = true;
        ReadingRequest_.Reset(new THashes());
        Stream_.Read(ReadingRequest_.Get(), new TRead(this));
    }

    void OnReaded() {
        THolder<THashes> request(new THashes());

        {
            auto g(Guard(Lock_));

            if (!Reading_) {
                return;
            }

            request.Swap(ReadingRequest_);
            Stream_.Read(ReadingRequest_.Get(), new TRead(this));
            HashesInProgress_ += request->HashesSize();
        }

        TThisRef ref(this);

        for (const TString& hash: request->GetHashes()) {
            Server_->ScheduleGetObject(hash, [this, ref] (const TString& h, const TString& b, const grpc::Status& status) {
                auto g(Guard(Lock_));

                WriteObjectNoLock(h, b, status);

                --HashesInProgress_;

                Y_UNUSED(ref);
            });
        }
    }

    void OnReadsDone() {
        auto g(Guard(Lock_));
        Reading_ = false;

        if (!Writing_ && HashesInProgress_ == 0) {
            Stream_.Finish(grpc::Status::OK, new TFinish(this));
        }
    }

    void OnWrited() {
        auto g(Guard(Lock_));

        Responses_.pop();
        Writing_ = ScheduleNextWriteNoLock();

        if (!Reading_ && !Writing_ && HashesInProgress_ == 0) {
            Stream_.Finish(grpc::Status::OK, new TFinish(this));
        }
    }

    void OnDone() {
        auto g(Guard(Lock_));
        Reading_ = false;
    }

private:
    void FlushResponseNoLock() {
        Y_ENSURE(Response_.DataSize());
        Responses_.emplace(std::move(Response_));
        Response_.ClearData();
        CurrentResponseSize_ = 0;
    }

    void WriteObjectNoLock(const TString& hash, const TString& blob, const grpc::Status& status) {
        if (status.error_code() != grpc::StatusCode::OK) {
            Response_.AddData(TString());  // sentinel for error
            Response_.AddData(hash);
            Response_.AddData(TString(status.error_message()));
        } else {
            Response_.AddData(hash);
            Response_.AddData(blob);
        }

        CurrentResponseSize_ += hash.size() + blob.size();

        if (CurrentResponseSize_ > 4 << 20 || status.error_code() != grpc::StatusCode::OK) {
            FlushResponseNoLock();
        }

        if (!Writing_) {
            Y_ENSURE(Writing_ = ScheduleNextWriteNoLock());
        }
    }

    bool ScheduleNextWriteNoLock() {
        if (Responses_.empty() && Response_.DataSize() == 0) {
            return false;
        }

        if (Responses_.empty()) {
            FlushResponseNoLock();
        }

        Stream_.Write(Responses_.front(), new TWrite(this));

        return true;
    }

private:
    TVcsServer* Server_;
    NVcs::Vcs::AsyncService* const Service_;
    grpc::ServerCompletionQueue* CQ_;
    grpc::ServerContext Ctx_;
    grpc::ServerAsyncReaderWriter<TObjects, THashes> Stream_;

    TMutex Lock_;
    THolder<THashes> ReadingRequest_;
    TObjects Response_;
    ui64 CurrentResponseSize_;
    TQueue<TObjects> Responses_;  // TODO: limit queue size(in bytes)
    ui64 HashesInProgress_;

    bool Writing_;
    bool Reading_;
};

} // namespace NAapi
