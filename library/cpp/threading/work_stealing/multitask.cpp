#include "multitask.h"
#include "queue.h"

inline void TMtpMultiTask::Delay(IObjectInQueue* job) {
    Delayed.push_back(job); // to be processed in current thread during Wait()
}

inline void TMtpMultiTask::Detach(IObjectInQueue* job) {
    Y_ASSERT(Processor);

    if (!Processor->Push(job))
        Delay(job);
}

void TMtpMultiTask::AddJob(IObjectInQueue* job) {
    // wrap each user job to enable waiting
    TWaitedMtpJob* wrapped = Jobs.Hold(new TWaitedMtpJob(job, *this));
    if (Started())
        Detach(wrapped); // should not block
}

void TMtpMultiTask::DoStart(TMtpJobsPtr processor, size_t minLocal) {
    if (Started())
        ythrow yexception() << "TMtpMultiTask: cannot start twice";

    Processor = processor;
    if (!Processor)
        Processor = new TDummyMtpJobs;

    for (size_t i = 0; i < Jobs.Size(); ++i) {
        int toDelay = (int)minLocal - Delayed.size();
        if (toDelay > 0 && i + toDelay >= Jobs.Size())
            Delay(Jobs[i]);
        else
            Detach(Jobs[i]);
    }

    Y_ASSERT(Delayed.size() >= Min(minLocal, Jobs.Size()));
}

inline void TMtpMultiTask::ProcessInCurrentThread(IObjectInQueue* job) {
    job->Process(nullptr);
    ++Done;
}

inline void TMtpMultiTask::ProcessDelayed() {
    for (IObjectInQueue* job : Delayed)
        ProcessInCurrentThread(job);

    Delayed.clear();
}

void TMtpMultiTask::CheckErrors() const {
    for (size_t i = 0; i < Jobs.Size(); ++i) {
        Jobs[i]->CheckError();
    }
}

void TMtpMultiTask::Wait() {
    if (!Started())
        ythrow yexception() << "TMtpMultiTask: has not started";

    ProcessDelayed();

    // Try pop and process in current thread as much as possible
    size_t total = Jobs.Size();
    while (Done < total) {
        if (IObjectInQueue* job = Processor->Pop())
            ProcessInCurrentThread(job);
        else
            break;
    }

    // If we have not done all our jobs yet, then some of them
    // are popped by other threads and we have to wait.
    if (Done < total)
        TMtpJobWaiter::WaitAll();

    Done = total;

    CheckErrors();
}

namespace {
    // Special proxy job queue: only pushes into IThreadPool
    class TMtpQueueProxy: public IMtpJobs {
    public:
        TMtpQueueProxy(IThreadPool* queue)
            : Queue(queue)
        {
        }

    private:
        bool DoPush(IObjectInQueue* job) override {
            return Queue->Add(job);
        }

        IObjectInQueue* DoPop() override {
            return nullptr; // never pops, IThreadPool does not support this operation.
        }

        IThreadPool* Queue;
    };
}

void TMtpMultiTask::Process(IThreadPool& processor) {
    DoStart(new TMtpQueueProxy(&processor), 1); // minLocal = 1
    Wait();
}

void TMtpMultiTask::Process(size_t threads) {
    if (threads <= 1)
        ProcessLocally();
    else {
        TFixedMtpQueue q;
        q.Start(threads - 1);
        Process(q.JobQueue());
    }
}
