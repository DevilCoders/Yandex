#include "server.h"

#include <util/generic/scope.h>
#include <util/generic/yexception.h>
#include <util/memory/pool.h>
#include <util/memory/smallobj.h>
#include <util/stream/mem.h>
#include <util/stream/multi.h>

#include <library/cpp/coroutine/engine/impl.h>
#include <library/cpp/coroutine/engine/sockpool.h>
#include <library/cpp/coroutine/listener/listen.h>
#include <library/cpp/deprecated/atomic/atomic.h>

#include <library/cpp/http/io/stream.h>

using namespace NCoroHttp;

using TConfig = THttpServer::TConfig;
using TCallbacks = THttpServer::TCallbacks;

struct THttpServer::TImpl: public TConfig, public TCallbacks, public TContListener::ICallBack {
    struct TShutdowner {
        inline TShutdowner(TImpl* parent, TContListener* listener)
            : Parent(parent)
            , Listener(listener)
        {
        }

        void operator()(TCont* c) {
            // Wait for shutdown notification
            while (!Parent->IsShuttingDown()) {
                c->SleepT(TDuration::MilliSeconds(500));
            }

            Listener->Stop();
            Parent->ShutdownImpl();
        }

        TImpl* Parent;
        TContListener* Listener;
    };

    struct TWorker: public TObjectFromPool<TWorker> {
        inline TWorker(TSocketHolder& s, TImpl* parent, const NAddr::IRemoteAddr* remoteAddress)
            : Socket(s.Release())
            , Parent(parent)
            , RemoteAddress(remoteAddress)
        {
        }

        void operator()(TCont* c) {
            try {
                THolder<TWorker> guard(this);
                TContIO io(Socket, c);
                bool again = true;
                char ch;

                while (again) {
                    if (NCoro::ReadI(c, io.Fd(), &ch, 1).Checked() == 0) {
                        break;
                    }

                    TMemoryInput i1(&ch, 1);
                    TMultiInput i2(&i1, &io);
                    THttpInput in(&i2);
                    THttpOutput out(&io, &in);

                    out.EnableKeepAlive(true);

                    if (!out.CanBeKeepAlive()) {
                        again = false;
                    }

                    TRequestContext ctx = {
                        &in,
                        &out,
                        &RemoteAddress,
                        c
                    };

                    if (Parent->OnRequestCb) {
                        Parent->OnRequestCb(ctx);
                    }

                    out.Finish();
                }
            } catch (...) {
                Parent->OnError();
            }
        }

        TSocketHolder Socket;
        TImpl* Parent;
        NAddr::TOpaqueAddr RemoteAddress;
    };

    inline TImpl(const TConfig& config, const TCallbacks& callbacks)
        : TConfig(config)
        , TCallbacks(callbacks)
        , Pool(0)
        , WPool(TDefaultAllocator::Instance())
        , ShutdownFlag(0)
    {
    }

    inline void RunCycle() {
        if (Executor) {
            DoRunCycle();
        } else {
            TContExecutor executor(CoroutineStackSize);
            Executor = &executor;

            DoRunCycle();
        }
    }

    inline void DoRunCycle() {
        TContListener listener(this, Executor, TContListener::TOptions().SetListenQueue(ListenQueueSize));
        TNetworkAddress address(!Host ? TNetworkAddress(Port) : TNetworkAddress(Host, Port));

        listener.Bind(address);
        listener.Listen();

        TShutdowner shutdowner(this, &listener);

        Executor->Create(shutdowner, "shutdowner");
        Executor->Execute();
    }

    void OnAcceptFull(const TAcceptFull& accept) override {
        THolder<TWorker> worker(new (&WPool) TWorker(*accept.S, this, accept.Remote));

        Executor->Create(*worker, "worker");
        Y_UNUSED(worker.Release());
    }

    void OnError() override {
        if (OnErrorCb) {
            OnErrorCb();
        }
    }

    inline void ShutDown() noexcept {
        // Notify shutdowner
        AtomicSet(ShutdownFlag, 1);
    }

    inline void ShutdownImpl() noexcept {
        Executor->Abort();
    }

    inline bool IsShuttingDown() const noexcept {
        return AtomicGet(ShutdownFlag) == 1;
    }

    TMemoryPool Pool;
    TWorker::TPool WPool;
    TAtomic ShutdownFlag;
};

THttpServer::THttpServer() {
}

THttpServer::~THttpServer() {
}

void THttpServer::ShutDown() {
    if (Impl_) {
        Impl_->ShutDown();
    }
}

void THttpServer::RunCycle(const TConfig& config, const TCallbacks& callbacks) {
    if (Impl_) {
        ythrow yexception() << TStringBuf("already running");
    }

    Y_DEFER {
        Impl_.Destroy();
    };

    Impl_.Reset(new TImpl(config, callbacks));
    Impl_->RunCycle();
}

TContExecutor* THttpServer::Executor() const noexcept {
    if (Impl_) {
        return Impl_->Executor;
    }

    return nullptr;
}

bool THttpServer::IsRunning() const noexcept {
    return (bool)Executor();
}
