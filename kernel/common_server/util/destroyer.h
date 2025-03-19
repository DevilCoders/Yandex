#pragma once

#include <library/cpp/balloc/optional/operators.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/ptr.h>
#include <util/system/yassert.h>
#include <util/thread/pool.h>

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/object_counter/object_counter.h>

template <class D>
struct TDummyCallback {
    static void BeforeDelete(D& /*obj*/) {
    }
};

template <class D, class TCallbackCaller = TDummyCallback<D>>
class TDestroyer {
private:
    TThreadPool Queue;
    TAtomic Running = 1;
    THolder<IThreadFactory::IThread> InfoSender;
    TString DestroyerId;

private:
    class TDestroyTask: public IObjectInQueue, NCSUtil::TObjectCounter<TDestroyTask> {
    private:
        D* Client;

    public:
        TDestroyTask(D* client) {
            CHECK_WITH_LOG(client);
            Client = client;
        }

        virtual void Process(void* /*ThreadSpecificResource*/) {
            ThreadDisableBalloc();
            TCallbackCaller::BeforeDelete(*Client);
            delete Client;
        }
    };

    class TDestroyTaskPtr: public IObjectInQueue {
    private:
        TAtomicSharedPtr<D> Client;

    public:
        TDestroyTaskPtr(TAtomicSharedPtr<D>&& client) {
            CHECK_WITH_LOG(client);
            Client = std::move(client);
        }

        virtual void Process(void* /*ThreadSpecificResource*/) {
            ThreadDisableBalloc();
            TCallbackCaller::BeforeDelete(*Client);
            Client = nullptr;
        }
    };

public:
    class TGuard {
    private:
        D* Obj;
        TDestroyer<D>& Destroyer;

    public:
        TGuard(TDestroyer<D>& destroyer, D* object)
            : Obj(object)
            , Destroyer(destroyer)
        {
        }

        ~TGuard() {
            Destroyer.Register(Obj);
        }
    };

public:
    TDestroyer(const TString& destroyerId)
        : DestroyerId(destroyerId)
    {
        Queue.Start(8);
        InfoSender = SystemThreadFactory()->Run([this]() {
            while (AtomicGet(Running)) {
                SendInfo();
                Sleep(TDuration::Seconds(1));
            }
        });
    }

    ~TDestroyer() {
        Queue.Stop();
        AtomicSet(Running, 0);
        if (InfoSender) {
            InfoSender->Join();
        }
        SendInfo();
    }

    void Register(D* client) {
        Y_VERIFY(Queue.AddAndOwn(MakeHolder<TDestroyTask>(client)), "Incorrect destroyer behaviour");
    }

    void Register(TAtomicSharedPtr<D>&& client) {
        Y_VERIFY(Queue.AddAndOwn(MakeHolder<TDestroyTaskPtr>(std::move(client))), "Incorrect destroyer behaviour");
    }

private:
    void SendInfo() const {
        TCSSignals::LTSignal("queue_size", Queue.Size())("object_type", CppDemangle(typeid(D).name()))("destroyer_id", DestroyerId);
    }
};
