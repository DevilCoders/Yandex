#include "httpsrv.h"

#include <library/cpp/coroutine/engine/impl.h>
#include <library/cpp/coroutine/listener/listen.h>
#include <library/cpp/http/server/http.h>

#include <util/generic/cast.h>
#include <util/network/ip.h>
#include <util/system/byteorder.h>
#include <util/system/event.h>
#include <util/thread/lfstack.h>
#include <util/thread/factory.h>

using namespace NHttp;

typedef NHttp::TRequest THttpRequest;

void NHttp::ServeHttpImpl(IRequester* t, const THttpServerOptions& options) {
    typedef THttpServer::ICallBack ICallBack;

    class TServer: public ICallBack, public THttpServer {
        class TRequest: public TClientRequest {
        public:
            bool Reply(void*) override {
                TIpAddress remote;
                TIpAddress local;

                Zero(remote);
                Zero(local);

                getsockname(Socket(), local, local);
                getpeername(Socket(), remote, remote);

                const THttpRequest req = {
                      &local
                    , &remote
                    , &Input()
                    , &Output()
                };

                static_cast<TServer*>(HttpServ())->T_->Reply(req);

                return true;
            }
        };

    public:
        inline TServer(IRequester* t, const THttpServerOptions& options)
            : THttpServer(this, options)
            , T_(t)
        {
        }

        TClientRequest* CreateClient() override {
            return new TRequest();
        }

        IRequester* T_;
    };

    TServer srv(t, options);

    srv.Start();
    srv.Wait();
}

static inline void Bind(THttpServerOptions opts, TSocketHolder& ret) {
    TSocketHolder s(::socket(AF_INET, SOCK_STREAM, 0));

    {
        int yes = 1;

        ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    }

    THttpServerOptions::TBindAddresses addrs;
    opts.BindAddresses(addrs);
    //TIpAddress sa = addrs[0];

    //::bind(s, sa, sa);
    ::listen(s, opts.ListenBacklog);

    ret.Swap(s);
}

void NHttp::ServeBasicHttpImpl(IRequester* r, const THttpServerOptions& options) {
    class TFunc: public IThreadFactory::IThreadAble {
    public:
        inline TFunc(IRequester* r, TSocketHolder* s)
            : R_(r)
            , S_(s)
        {
        }

        void DoExecute() override {
            while (true) {
                try {
                    TIpAddress remote;
                    TIpAddress local;

                    Zero(remote);
                    Zero(local);

                    TSocket s(::accept(*S_, remote, remote));

                    getsockname(s, local, local);

                    THttpServerConn conn(s);

                    const THttpRequest req = {
                          &local
                        , &remote
                        , conn.Input()
                        , conn.Output()
                    };

                    R_->Reply(req);
                } catch (...) {
                    Cerr << CurrentExceptionMessage() << Endl;
                }
            }
        }

    private:
        IRequester* R_;
        TSocketHolder* S_;
    };

    TSocketHolder s(INVALID_SOCKET);

    Bind(options, s);

    IThreadFactory* p = SystemThreadFactory();
    TVector<TAutoPtr<IThreadFactory::IThread> > thrs;
    TFunc func(r, &s);

    for (size_t i = 0; i < options.nThreads; ++i) {
        thrs.push_back(p->Run(&func));
    }

    for (size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->Join();
    }
}

namespace {
    class TUnqueuedHttp: public TContListener::ICallBack  {
        struct TConn {
            inline TConn(const TAccept& acc, TUnqueuedHttp* parent) noexcept
                : Local(*acc.Local)
                , Remote(*acc.Remote)
                , Socket(acc.S->Release())
                , Parent(parent)
            {
            }

            inline void Process() {
                SetNonBlock(Socket, false);

                THttpServerConn conn(Socket);

                const THttpRequest req = {
                      &Local
                    , &Remote
                    , conn.Input()
                    , conn.Output()
                };

                Parent->R_->Reply(req);
            }

            TIpAddress Local;
            TIpAddress Remote;
            TSocket Socket;
            TUnqueuedHttp* Parent;
        };

        typedef TAutoPtr<TConn> TConnRef;

        template <class T>
        class TThr: public IThreadFactory::IThreadAble {
            typedef TAutoPtr<T> TRef;
        public:
            inline TThr(TUnqueuedHttp* parent)
                : E_(TSystemEvent::rAuto)
                , P_(parent)
                , Thr_(P_->Pool()->Run(this))
            {
            }

            inline void Schedule(TRef t) noexcept {
                T_.Swap(t);
                E_.Signal();
            }

        private:
            void DoExecute() override {
                while (true) {
                    while (!T_) {
                        E_.Wait();
                    }

                    try {
                        T_->Process();
                    } catch (...) {
                        Cerr << CurrentExceptionMessage() << Endl;
                    }

                    T_.Destroy();
                    P_->Avail(this);
                }
            }

        private:
            TRef T_;
            TSystemEvent E_;
            TUnqueuedHttp* P_;
            TAutoPtr<IThreadFactory::IThread> Thr_;
        };

        typedef TThr<TConn> TConnThr;
        typedef TAutoPtr<TConnThr> TConnThrRef;
        typedef TVector<TConnThrRef> TThrs;

    public:
        inline TUnqueuedHttp(IRequester* r, const THttpServerOptions& options)
            : R_(r)
            , O_(&options)
        {
        }

        inline void Serve() {
            TContExecutor executor(8000);
            TContListener listener(this, &executor);

            typedef THttpServerOptions::TBindAddresses TAddrs;
            TAddrs addrs;

            O_->BindAddresses(addrs);

            for (TAddrs::const_iterator it = addrs.begin(); it != addrs.end(); ++it) {
                listener.Bind(*it);
            }

            listener.Listen();
            executor.Execute();
        }

        inline IThreadFactory* Pool() const noexcept {
            return SystemThreadFactory();
        }

    private:
        void OnAccept(const TAccept& acc) override {
            TConnThr* avail = FindAvail();

            if (avail) {
                avail->Schedule(new TConn(acc, this));
            }
        }

        void OnError() override {
            Cerr << CurrentExceptionMessage() << Endl;
        }

        inline TConnThr* FindAvail() {
            TConnThr* ret = nullptr;

            if (A_.Dequeue(&ret)) {
                return ret;
            }

            return NewThread();
        }

        inline void Avail(TConnThr* thr) {
            A_.Enqueue(thr);
        }

        inline TConnThr* NewThread() {
            const size_t maxthr = O_->nThreads;

            if (maxthr && T_.size() >= maxthr) {
                return nullptr;
            }

            T_.push_back(new TConnThr(this));

            return T_.back().Get();
        }

    private:
        IRequester* R_;
        const THttpServerOptions* O_;
        TThrs T_;
        TLockFreeStack<TConnThr*> A_;
    };
}

void NHttp::ServeUnqueuedHttpImpl(IRequester* r, const THttpServerOptions& options) {
    TUnqueuedHttp(r, options).Serve();
}

template <>
void Out<TIpAddress>(IOutputStream& out, const TIpAddress& addr) {
    out << IpToString(addr.sin_addr.s_addr) << ":" << InetToHost(addr.sin_port);
}
