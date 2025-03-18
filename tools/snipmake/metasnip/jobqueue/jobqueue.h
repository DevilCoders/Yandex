#pragma once

#include <util/thread/pool.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/generic/deque.h>
#include <util/generic/ptr.h>

namespace NSnippets {
    struct IAcceptJobDone {
        virtual void Receive(size_t id) = 0;
    };
    struct TJobReport {
        IAcceptJobDone* Master;
        size_t Id;
        TJobReport()
          : Master(nullptr)
          , Id(size_t(-1))
        {
        }
        void Clear() {
            Master = nullptr;
            Id = size_t(-1);
        }
        void Deliver() const {
            if (Master) {
                Master->Receive(Id);
            }
        }
    };
    class TJobQueue : public IAcceptJobDone {
    public:
        typedef TAtomicSharedPtr<IObjectInQueue> TJobPtr;
    private:
        IThreadPool* const Impl;
        TMutex DoneMutex;
        TCondVar DoneCond;
        TDeque<bool> Done;

        TDeque<TJobPtr> Jobs;
        size_t Offset;
        size_t MaxQueue;
    public:
        TJobQueue(IThreadPool* impl)
          : Impl(impl)
          , Offset(0)
          , MaxQueue(0)
        {
        }
        virtual ~TJobQueue() {
        }
        void Start(size_t threads, size_t maxQueueSlave = 0, size_t maxQueueSelf = 0) {
            MaxQueue = maxQueueSelf;
            Impl->Start(threads, maxQueueSlave);
        }
        bool Add(TJobPtr obj, TJobReport* report) {
            TGuard<TMutex> g(&DoneMutex);
            if (MaxQueue && Done.size() >= MaxQueue) {
                return false;
            }
            const size_t id = Offset + Jobs.size();
            Jobs.push_back(obj);
            Done.push_back(false);
            if (report) {
                report->Id = id;
                report->Master = this;
            }
            const bool add = Impl->Add(obj.Get());
            if (!add) {
                Jobs.pop_back();
                Done.pop_back();
                if (report) {
                    report->Clear();
                }
            }
            return add;
        }
        void Receive(size_t id) override {
            {
                TGuard<TMutex> g(&DoneMutex);
                Done[id - Offset] = true;
            }
            DoneCond.Signal();
        }
        TJobPtr Complete(size_t i) {
            TGuard<TMutex> g(&DoneMutex);
            while (i - Offset < Done.size() && !Done[i - Offset]) {
                DoneCond.WaitI(DoneMutex);
            }
            if (i - Offset >= Done.size()) {
                return nullptr;
            }
            TJobPtr res = Jobs[i - Offset];
            Done.pop_front();
            Jobs.pop_front();
            ++Offset;
            return res;
        }
        TJobPtr CompleteFront() {
            return Complete(Offset);
        }
        void Stop() {
            Impl->Stop();
        }
    };
    struct TReportGuard {
        TJobReport* Report;
        TReportGuard(TJobReport* report)
          : Report(report)
        {
        }
        ~TReportGuard() {
            Report->Deliver();
        }
    };
}
