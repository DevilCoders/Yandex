#include "threaded.h"

#include <util/generic/scope.h>

#include <pthread.h>

namespace NFuse {


TThreadedFuseLoop::TThreadedFuseLoop(
    TMountProcessorRef mount,
    size_t ioThreads,
    TLog log,
    TIntrusivePtr<NMonitoring::TDynamicCounters> counters
)
    : Mount_(mount)
    , IoThreadCount_(ioThreads)
    , Counters_(counters)
    , Log_(log)
    , Stop_(0)
{
    // Install a no-op signal handler (see TThreadedFuseLoop::Stop()).
    // XXX do it only once per process.
    struct sigaction sa = {};
    sa.sa_handler = [](int) {};
    sigaction(SIGUSR1, &sa, nullptr);

    mount->SetIOThreadCount(IoThreadCount_);
    for (size_t i = 0; i < IoThreadCount_; i++) {
        PollThreads_.push_back(SystemThreadFactory()->Run([this, i] { DoExecute(i); }));
    }
}

TThreadedFuseLoop::~TThreadedFuseLoop() {
    Stop();
    WaitForStop();
}

void TThreadedFuseLoop::Stop() {
    AtomicSet(Stop_, 1);

    // We need to interrupt threads, as they might be stuck on read call to
    // fuse fd. This is racy as signal might get received right before
    // read call.
    with_lock(Lock_) {
        for (auto tid : ThreadIds_) {
            pthread_kill(tid, SIGUSR1);
        }
    }
}

void TThreadedFuseLoop::WaitForStop() {
    for (auto& thread : PollThreads_) {
        thread->Join();
    }
}

void TThreadedFuseLoop::DoExecute(int threadNum) {
    Log_ << TLOG_INFO << "Starting fuse io loop thread " << threadNum << Endl;
    TThread::SetCurrentThreadName("fuse_loop");

    pthread_t tid = pthread_self();
    with_lock(Lock_) {
        ThreadIds_.emplace(tid);
    }
    Y_DEFER {
        with_lock(Lock_) {
            ThreadIds_.erase(tid);
        }
    };

    TBuffer buf;

    while (AtomicGet(Stop_) == 0) {
        try {
            auto status = Mount_->Receive(buf);
            switch (status) {
            case EMessageStatus::Success:
                Counters_.ActiveThreads->Inc();
                Mount_->Process(TStringBuf{buf.Data(), buf.Size()}, threadNum);
                Counters_.ActiveThreads->Dec();
                break;
            case EMessageStatus::Stop:
                Log_ << TLOG_INFO << "Stopping fuse io loop thread" << Endl;
                Stop();
                return;
            case EMessageStatus::Again:
                break;
            default:
                const auto path = Mount_->GetMountPath();
                const auto fd = Mount_->GetChannelFd();
                Log_ << TLOG_ERR << "Unexpected message status "
                     << LabeledOutput(path, fd, status) << Endl;
            }
        } catch (...) {
            TString path = Mount_->GetMountPath();
            int fd = Mount_->GetChannelFd();
            TString msg = CurrentExceptionMessage();
            Log_ << TLOG_ERR << "Exception while processing fuse request for "
                 << LabeledOutput(path, fd, msg) << Endl;
        }
    }
}

} // namespace NVcs::NFuse
