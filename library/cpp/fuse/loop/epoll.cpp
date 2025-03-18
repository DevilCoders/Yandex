#include "epoll.h"
#include "counters.h"

#include <library/cpp/containers/concurrent_hash/concurrent_hash.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/thread/factory.h>
#include <util/thread/pool.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

#ifndef EPOLLEXCLUSIVE
#define EPOLLEXCLUSIVE (1 << 28)
#endif

namespace NFuse {

class TEpollFuseLoop::TImpl {
public:
    TImpl(
        size_t numThreads,
        TLog log,
        TIntrusivePtr<NMonitoring::TDynamicCounters> counters
    );
    ~TImpl();

    void Run();

    TMountHandle AttachMount(TMountProcessorRef mount);
    void DetachMount(TMountHandle handle);

private:
    void DoExecute(int epollFd, int threadIdx);

private:
    const size_t IoThreadCount_;

    TVector<int> EpollFds_;
    TAtomic Stop_;

    TFuseLoopCounters Counters_;
    TLog Log_;

    using IThreadRef = THolder<IThreadFactory::IThread>;
    TVector<IThreadRef> PollThreads_;

    std::atomic<TMountHandle> LastHandle_;
    TConcurrentHashMap<TMountHandle, TMountProcessorRef, 16, TMutex> Mounts_;
};

TEpollFuseLoop::TImpl::TImpl(
    size_t ioThreads,
    TLog log,
    TIntrusivePtr<NMonitoring::TDynamicCounters> counters
)
    : IoThreadCount_(ioThreads)
    , Stop_(0)
    , Counters_(counters)
    , Log_(log)
    , LastHandle_(0)
{
    EpollFds_.resize(IoThreadCount_);
    for (size_t i = 0; i < IoThreadCount_; i++) {
        EpollFds_[i] = epoll_create1(EPOLL_CLOEXEC);
        if (EpollFds_[i] < 0) {
            ythrow TSystemError() << "failed to create epoll instance";
        }
    }

    Run();
}

TEpollFuseLoop::TImpl::~TImpl() {
    AtomicSet(Stop_, 1);
    for (auto& thread : PollThreads_) {
        thread->Join();
    }
    for (auto& fd : EpollFds_) {
        close(fd);
    }
}

void TEpollFuseLoop::TImpl::Run() {
    Y_ENSURE(PollThreads_.empty());

    for (size_t i = 0; i < IoThreadCount_; i++) {
        PollThreads_.push_back(SystemThreadFactory()->Run([this, fd = EpollFds_[i], i] { DoExecute(fd, i); }));
    }
}

void TEpollFuseLoop::TImpl::DoExecute(int epollFd, int threadIdx) {
    constexpr int MAX_EVENTS = 4;
    Log_ << TLOG_INFO << "Starting MountMux io loop thread" << Endl;
    TThread::SetCurrentThreadName("TMountMux");

    struct epoll_event events[MAX_EVENTS];
    TVector<TMountProcessorRef> activeMounts;
    activeMounts.reserve(MAX_EVENTS);

    TBuffer buf;

    while (AtomicGet(Stop_) == 0) {
        int eventCount = epoll_wait(epollFd, events, MAX_EVENTS, 1000);
        if (eventCount < 0) {
            if (errno != EINTR) {
                ythrow TSystemError() << "epoll_wait failed";
            }
        }

        Counters_.RequestsPending->Add(eventCount);
        Counters_.ActiveThreads->Inc();

        for (int i = 0; i < eventCount; i++) {
            struct epoll_event* event = &events[i];

            TMountHandle handle = event->data.u64;
            TMountProcessorRef mount;
            if (Mounts_.Get(handle, mount)) {
                activeMounts.push_back(std::move(mount));
            }
        }

        while (!activeMounts.empty()) {
            auto it = activeMounts.begin();
            while (it != activeMounts.end()) {
                auto& mount = *it;
                try {
                    if (mount->Receive(buf) == EMessageStatus::Success) {
                        mount->Process(TStringBuf{buf.Data(), buf.Size()}, threadIdx);
                        ++it;
                    } else {
                        //TODO: notify about stop & errors
                        it = activeMounts.erase(it);
                        Counters_.RequestsPending->Dec();
                    }
                } catch (...) {
                    TString path = mount->GetMountPath();
                    int fd = mount->GetChannelFd();
                    TString msg = CurrentExceptionMessage();
                    Log_ << TLOG_ERR << "Exception while processing fuse request for "
                         << LabeledOutput(path, fd, msg) << Endl;
                }
            }
        }

        Counters_.ActiveThreads->Dec();
    }
}

TMountHandle TEpollFuseLoop::TImpl::AttachMount(TMountProcessorRef processor) {
    TMountHandle handle = ++LastHandle_;
    Mounts_.InsertUnique(handle, processor);

    const int fd = processor->GetChannelFd();
    const TString path = processor->GetMountPath();
    Log_ << TLOG_INFO << "Attaching mount " << LabeledOutput(path, handle, fd) << Endl;
    processor->SetIOThreadCount(IoThreadCount_);

    const int flags = fcntl(fd, F_GETFL);
    if ((flags & O_NONBLOCK) != O_NONBLOCK) {
        const int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (ret != 0) {
            ythrow TSystemError() << "failed to switch channel to nonblocking mode";
        }
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLEXCLUSIVE | EPOLLET;
    event.data.u64 = handle;

    for (size_t i = 0; i < IoThreadCount_; i++) {
        int ret = epoll_ctl(EpollFds_[i], EPOLL_CTL_ADD, fd, &event);
        if (ret != 0) {
            ythrow TSystemError() << "epoll_ctl failed";
        }
    }

    return handle;
}

void TEpollFuseLoop::TImpl::DetachMount(TMountHandle handle) {
    TMountProcessorRef processor;
    if (!Mounts_.Get(handle, processor)) {
        Log_ << TLOG_ERR << "Trying to detach unknown mount " << handle << Endl;
        return;
    }
    Mounts_.Remove(handle);

    const int fd = processor->GetChannelFd();
    const TString path = processor->GetMountPath();
    const auto fdPath = (TFsPath("/proc/self/fd") / ToString(fd)).ReadLink();
    Log_ << TLOG_INFO << "Detaching mount " << LabeledOutput(path, handle, fd, fdPath) << Endl;

    for (size_t i = 0; i < IoThreadCount_; i++) {
        int ret = epoll_ctl(EpollFds_[i], EPOLL_CTL_DEL, fd, nullptr);
        if (ret != 0) {
            ythrow TSystemError() << "epoll_ctl failed";
        }
    }
}

TEpollFuseLoop::TEpollFuseLoop(
    size_t ioThreads,
    TLog log,
    TIntrusivePtr<NMonitoring::TDynamicCounters> counters
)
    : Impl_(MakeHolder<TImpl>(ioThreads, log, counters))
{
}

TEpollFuseLoop::~TEpollFuseLoop()
{
}

void TEpollFuseLoop::Run() {
    Impl_->Run();
}

TMountHandle TEpollFuseLoop::AttachMount(TMountProcessorRef processor) {
    return Impl_->AttachMount(processor);
}

void TEpollFuseLoop::DetachMount(TMountHandle handle) {
    Impl_->DetachMount(handle);
}

} // namespace NVcs::NFuse
