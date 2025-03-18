#include "worker.h"

#include <library/cpp/threading/future/async.h>
#include <util/generic/maybe.h>
#include <util/network/poller.h>
#include <util/network/sock.h>
#include <util/network/socket.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/system/event.h>
#include <util/system/thread.h>

namespace NUdp::NPrivate {
    using TError = TString;

    class TWorker::TImpl {
    public:
        TImpl(const TServerOptions& options,
              TCallback callback,
              size_t workerId)
            : Options_(options)
            , Callback_(callback)
            , WorkerId_(workerId)
        {
        }

        ~TImpl() {
            try {
                Stop();
            } catch (...) {
            }
        }

        void Init() {
            WorkingThreadPool_.Start(1);
            WorkerThread_ = NThreading::Async([&]() { return Worker(); }, WorkingThreadPool_);
            for (;;) {
                if (WorkerIsReady_.WaitT(Options_.PollTimeout)) {
                    Inited_ = true;
                    return;
                }
                if (WorkerThread_.HasValue()) {
                    const auto maybeError = WorkerThread_.GetValueSync();
                    Y_VERIFY(maybeError.Defined());
                    ythrow yexception() << *maybeError;
                }
            }
        }

        void Start() {
            if (!Inited_) {
                ythrow yexception() << "worker is not inited";
            }
            WorkerStartSignal_.Signal();
        }

        void WaitForWorkerRunning() {
            for (;;) {
                if (WorkerIsRunning_.WaitT(Options_.PollTimeout)) {
                    return;
                }
                if (WorkerThread_.HasValue()) {
                    const auto maybeError = WorkerThread_.GetValueSync();
                    Y_VERIFY(maybeError.Defined());
                    ythrow yexception() << *maybeError;
                }
            }
        }

        void Wait() {
            const auto maybeError = WorkerThread_.GetValueSync();
            if (maybeError) {
                ythrow yexception() << *maybeError;
            }
        }

        void Stop() {
            ShouldStop_ = true;
            Wait();
            WorkerIsRunning_.Reset();
        }

        void SetErrorCallback(TErrorCallback callback) {
            ErrorCallback_ = callback;
        }

    private:
        TMaybe<TError> Worker() {
            try {
                WorkerLoop();
            } catch (const yexception& ex) {
                return ex.what();
            }
            return {};
        }

        void BindSocket(TInet6DgramSocket& socket) {
            TSockAddrInet6 address("", Options_.Port);
            int result = address.Bind(socket, 0);
            if (result != 0) {
                ythrow yexception() << Sprintf("bind() failed with %s", LastSystemErrorText());
            }
        }

        void WorkerLoop() {
            const TString threadName = TString::Join("UDP server worker ", ToString(WorkerId_));
            TThread::SetCurrentThreadName(threadName.data());
            TInet6DgramSocket socket;
            if (Options_.Workers > 1) {
                SetReusePort(socket, true);
            }
            WorkerIsReady_.Signal();
            for (;;) {
                if (WorkerStartSignal_.WaitT(Options_.PollTimeout)) {
                    break;
                }
                if (ShouldStop_) {
                    return;
                }
            }

            BindSocket(socket);
            TSocketPoller socketPoller;
            socketPoller.WaitRead(socket, nullptr);
            WorkerIsRunning_.Signal();
            TVector<char> data(UdpPacketSize);

            for (;;) {
                void* pollerRet;
                if (socketPoller.WaitT(&pollerRet, 1, Options_.PollTimeout) == 0) {
                    if (ShouldStop_) {
                        break;
                    } else {
                        continue;
                    }
                }
                TUdpPacket packet;
                ssize_t packetSize = socket.RecvFrom(data.data(), UdpPacketSize, &packet.Address);
                if (packetSize < 0) {
                    if (ErrorCallback_) {
                        TString errorMessage = Sprintf("recvfrom() failed with %s", LastSystemErrorText());
                        ErrorCallback_(std::move(errorMessage));
                    }
                    continue;
                }
                packet.Data = TString(data.data(), packetSize);
                Callback_(std::move(packet));
            }
        }

        const TServerOptions Options_;
        TCallback Callback_;
        TErrorCallback ErrorCallback_;
        size_t WorkerId_;
        TThreadPool WorkingThreadPool_;
        NThreading::TFuture<TMaybe<TError>> WorkerThread_;
        TManualEvent WorkerIsReady_;
        TManualEvent WorkerStartSignal_;
        TManualEvent WorkerIsRunning_;
        bool Inited_ = false;
        bool ShouldStop_ = false;
    };

    TWorker::TWorker(const TServerOptions& options,
                     TCallback callback,
                     size_t workerId)
        : Impl_(new TImpl(options, callback, workerId))
    {
    }

    TWorker::~TWorker() = default;

    void TWorker::Init() {
        Impl_->Init();
    }

    void TWorker::Start() {
        Impl_->Start();
    }

    void TWorker::WaitForWorkerRunning() {
        Impl_->WaitForWorkerRunning();
    }

    void TWorker::Wait() {
        Impl_->Wait();
    }

    void TWorker::Stop() {
        Impl_->Stop();
    }

    void TWorker::SetErrorCallback(TErrorCallback callback) {
        Impl_->SetErrorCallback(callback);
    }

}
