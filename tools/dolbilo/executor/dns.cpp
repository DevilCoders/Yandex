#include "dns.h"

#include <library/cpp/coroutine/engine/impl.h>

#include <util/generic/intrlist.h>
#include <util/generic/yexception.h>
#include <util/thread/pool.h>

static const TString UNKNOWN_ERROR("unknown dns error");

class TPipeEvent: public TIntrusiveListItem<TPipeEvent> {
    public:
        inline TPipeEvent()
            : Done_(false)
        {
        }

        inline ~TPipeEvent() {
        }

        inline void Wake() noexcept {
            Done_ = true;
        }

        inline void Wait(TCont* c) noexcept {
            while (!Done_) {
                c->SleepT(TDuration::MilliSeconds(1));
            }
        }

        inline void Reset() noexcept {
            Done_ = false;
        }

    private:
        bool Done_;
};

class TAsyncDns::TImpl {
        class TResolveState: public IObjectInQueue {
            public:
                inline TResolveState(TImpl* parent, const TString& host, ui16 port)
                    : Parent_(parent)
                    , Event_(Parent_->AcquireEvent())
                    , Host_(host)
                    , Port_(port)
                {
                }

                inline ~TResolveState() override {
                }

                inline TAddrRef Execute() {
                    if (!Parent_->Queue_.Add(this)) {
                        ythrow yexception() << "can not resolve(queue full)";
                    }

                    Event_->Wait(Parent_->Executor_->Running());

                    if (!Resolved_ && !Error_.empty()) {
                        ythrow yexception() << "can not resolve(" <<  Error_.data() << ")";
                    }

                    Parent_->ReleaseEvent(Event_);

                    return Resolved_;
                }

            private:
                void Process(void* /*tsr*/) override {
                    try {
                        Resolved_ = TSyncDns().Resolve(Host_, Port_);
                    } catch (...) {
                        try {
                            Error_ = CurrentExceptionMessage();
                        } catch (...) {
                            Error_ = UNKNOWN_ERROR;
                        }
                    }

                    Event_->Wake();
                }

            private:
                TImpl* Parent_;
                THolder<TPipeEvent> Event_;
                const TString& Host_;
                const ui16 Port_;
                TAddrRef Resolved_;
                TString Error_;
        };

    public:
        inline TImpl(TContExecutor* e, size_t max)
            : Executor_(e)
        {
            Queue_.Start(max);
        }

        inline ~TImpl() {
            Wait();

            while (!Events_.Empty()) {
                delete Events_.PopBack();
            }
        }

        inline TAddrRef Resolve(const TString& host, ui16 port) {
            TResolveState state(this, host, port);

            return state.Execute();
        }

        inline TPipeEvent* AcquireEvent() {
            if (Events_.Empty()) {
                return new TPipeEvent();
            }

            return Events_.PopBack();
        }

        inline void ReleaseEvent(TAutoPtr<TPipeEvent> event) noexcept {
            event->Reset();
            Events_.PushBack(event.Release());
        }

    private:
        inline void Wait() noexcept {
            Queue_.Stop();
        }

    private:
        TContExecutor* Executor_;
        TSimpleThreadPool Queue_;
        TIntrusiveList<TPipeEvent> Events_;
};

TAsyncDns::TAsyncDns(TContExecutor* e) noexcept
    : Impl_(new TImpl(e, 0))
{
}

TAsyncDns::TAsyncDns(TContExecutor* e, size_t maxreqs) noexcept
    : Impl_(new TImpl(e, maxreqs))
{
}

TAsyncDns::~TAsyncDns() {
}

IDns::TAddrRef TAsyncDns::Resolve(const TString& host, ui16 port) {
    return Impl_->Resolve(host, port);
}
