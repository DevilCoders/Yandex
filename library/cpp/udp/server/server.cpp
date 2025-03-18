#include "server.h"
#include "worker.h"

#include <library/cpp/threading/future/async.h>

#include <util/thread/pool.h>

namespace NUdp {
    class TUdpServer::TImpl {
    public:
        TImpl(const TServerOptions& options,
              TCallback callback)
            : Options_(options)
            , Callback_(callback)
        {
            if (options.Executors > 0) {
                ExecutorsPool_.Start(options.Executors, options.QueueSizeLimit);
            }
            if (options.ErrorThreads > 0) {
                ErrorHandlersPool_.Start(options.ErrorThreads, options.ErrorQueueSizeLimit);
            }
            for (size_t workerIndex = 0; workerIndex < options.Workers; ++workerIndex) {
                Workers_.emplace_back(new NPrivate::TWorker(options, [this](TUdpPacket packet) { RunExecutor(std::move(packet)); }, workerIndex));
                if (ErrorCallback_) {
                    Workers_.back()->SetErrorCallback([this](TString error) {
                        try {
                            ErrorHandlersPool_.SafeAddFunc([=]() { ErrorCallback_(std::move(error)); });
                        } catch (...) {
                        }
                    });
                }
            }
        }

        void Start() {
            if (Workers_.size() > 1) {
                Y_VERIFY(IsReusePortAvailable(), "SO_REUSEPORT must be supported by kernel for using more than 1 worker");
            }
            for (auto& worker : Workers_) {
                worker->Init();
            }
            for (auto& worker : Workers_) {
                worker->Start();
            }
            for (auto& worker : Workers_) {
                worker->WaitForWorkerRunning();
            }
        }

        void Wait() {
            for (auto& worker : Workers_) {
                worker->Wait();
            }
        }

        void Stop() {
            for (auto& worker : Workers_) {
                worker->Stop();
            }
        }

        void SetErrorCallback(TErrorCallback callback) {
            ErrorCallback_ = callback;
        }

        void SetQueueOverflowCallback(TQueueOverflowCallback callback) {
            QueueOverflowCallback_ = callback;
        }

    private:
        void RunExecutor(TUdpPacket packet) {
            if (Options_.Executors > 0) {
                if (!ExecutorsPool_.AddFunc([=]() { Callback_(packet); })) {
                    if (QueueOverflowCallback_) {
                        try {
                            ErrorHandlersPool_.SafeAddFunc([=]() { QueueOverflowCallback_(std::move(packet)); });
                        } catch (...) {
                        }
                    }
                }
            } else {
                Callback_(std::move(packet));
            }
        }

        TServerOptions Options_;
        TCallback Callback_;
        TErrorCallback ErrorCallback_;
        TQueueOverflowCallback QueueOverflowCallback_;
        TVector<THolder<NPrivate::TWorker>> Workers_;
        TThreadPool ExecutorsPool_;
        TThreadPool ErrorHandlersPool_;
    };

    TUdpServer::TUdpServer(const TServerOptions& options,
                           TCallback callback)
        : Impl_(new TImpl(options, callback))
    {
    }

    TUdpServer::~TUdpServer() {
        try {
            Stop();
        } catch (...) {
        }
    }

    void TUdpServer::Start() {
        Impl_->Start();
    }

    void TUdpServer::Wait() {
        Impl_->Wait();
    }

    void TUdpServer::Stop() {
        Impl_->Stop();
    }

    void TUdpServer::SetErrorCallback(TErrorCallback callback) {
        Impl_->SetErrorCallback(callback);
    }

    void TUdpServer::SetQueueOverflowCallback(TQueueOverflowCallback callback) {
        Impl_->SetQueueOverflowCallback(callback);
    }

}
