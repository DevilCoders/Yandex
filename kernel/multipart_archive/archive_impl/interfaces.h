#pragma once

#include <util/system/types.h>
#include <util/thread/pool.h>
#include <util/system/condvar.h>

#include <library/cpp/deprecated/atomic/atomic.h>
#include <library/cpp/logger/global/global.h>

namespace NRTYArchive {
    class IPartCallBack {
    public:
        virtual ~IPartCallBack() {}

        virtual void OnPartFree(ui64 partNum) = 0;
        virtual void OnPartFull(ui64 partNum) = 0;
        virtual void OnPartDrop(ui64 partNum) = 0;
        virtual void OnPartClose(ui64 partNum, ui64 docsCount) = 0;
    };

    class TLinksCounter {
    public:
        virtual ~TLinksCounter() {}

        TLinksCounter() = default;

        void RegisterLink() {
            AtomicIncrement(LinkedObjectsCount);
        }

        void UnRegisterLink() {
            TGuard<TMutex> g(Lock);
            if (AtomicDecrement(LinkedObjectsCount) == 0) {
                CondVar.Signal();
            }
        }

        void WaitAllTasks(i64 maxTasksCount = 0) {
            TGuard<TMutex> g(Lock);
            while (AtomicGet(LinkedObjectsCount) > maxTasksCount) {
                DEBUG_LOG << "Wait " << AtomicGet(LinkedObjectsCount) << Endl;
                CondVar.Wait(Lock);
            }
        }

        bool Check() const {
            return AtomicGet(LinkedObjectsCount) > 0;
        }

    private:
        TMutex Lock;
        TCondVar CondVar;
        TAtomic LinkedObjectsCount = 0;
    };

    class ILinkedTask : public IObjectInQueue {
    private:
        TLinksCounter& Owner;

    public:
        ILinkedTask(TLinksCounter& owner)
            : Owner(owner)
        {
            Owner.RegisterLink();
        }

        virtual void Process(void* threadSpecificResource) override final {
            DoProcess(threadSpecificResource);
            Owner.UnRegisterLink();
        }

    private:
        virtual void DoProcess(void* /*threadSpecificResource*/) = 0;
    };
}
