#pragma once
#include <util/system/thread.h>
#include <util/system/event.h>
#include <util/system/sigset.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

#ifdef WIN32
// Define the missing signals to simplify usage
#define SIGHUP  0
#define SIGUSR1 0
#endif

namespace NUtil {

    typedef void (*TSigHandlerProc)(int);
    typedef std::pair<int, TSigHandlerProc> TSigHandler;

    class TSigHandlerThread : public TThread {
    private:
        typedef TSet<int> TSigSet;
        typedef TVector<int> TSigList;
        typedef TMap<TSigHandlerProc, TSigList> TSigHandlerMap;

    public:
        TSigHandlerThread()
            : TThread(ThreadProc, this)
        {}
        TSigHandlerThread(const TVector<TSigHandler>& sigHandlers)
            : TThread(ThreadProc, this)
        {
            for (TVector<TSigHandler>::const_iterator itr = sigHandlers.begin(); itr != sigHandlers.end(); ++itr) {
                int sig = itr->first;
                TSigHandlerProc handler = itr->second;
                if (!!sig && !!handler) {
                    Y_VERIFY(SigSet.insert(sig).second, "Duplicate signal %d in sig handlers", sig);
                    SigHandlerMap[handler].push_back(sig);
                }
            }
        }
        ~TSigHandlerThread() {
            Stop();
        }
        static void* ThreadProc(void* _this) {
            static_cast<TSigHandlerThread*>(_this)->Run();
            return nullptr;
        }
        void Stop() {
            Quit.Signal();
            Join();
        }

        void DelegateSignals() {
            if (!Running() && !SigHandlerMap.empty()) {
                Start(); // start dedicated signal handler thread
                SetSigMask(SIG_BLOCK, nullptr); // block signals handled by the started thread
            }
        }

    private:
        void Run() {
            if (SigHandlerMap.empty()) {
                return;
            }

#ifndef WIN32
            // To maximize async signal safety, no malloc/free/printf/... unsafe operations should be
            // allowed in this thread context (other than the signal handlers, but sigaction guarantees
            // serialized triggering, as long as the same signal handler is not used in other threads).
            struct sigaction sa;
            sa.sa_flags = 0;
            SigEmptySet(&sa.sa_mask);
            for (TSigSet::const_iterator itr = SigSet.begin(); itr != SigSet.end(); ++itr) {
                SigAddSet(&sa.sa_mask, *itr);
            }
            for (TSigHandlerMap::const_iterator itr = SigHandlerMap.begin(); itr != SigHandlerMap.end(); ++itr) {
                TSigHandlerProc handler = itr->first;
                const TSigList& siglist = itr->second;
                sa.sa_handler = handler;
                for (TSigList::const_iterator sig = siglist.begin(); sig != siglist.end(); ++sig) {
                    sigaction(*sig, &sa, nullptr);
                }
            }
            // Unblock the signals I can handle:
            // Because different threads can handle the same signal with different (or same) handlers,
            // always unblock the ones I can handle, even if it's initially blocked (inherited from parent)
            SetSigMask(SIG_UNBLOCK, nullptr);

            // Wait until I'm stopped
            while (!Quit.WaitT(TDuration::Seconds(60)));
#else
            for (TSigHandlerMap::const_iterator itr = SigHandlerMap.begin(); itr != SigHandlerMap.end(); ++itr) {
                // Not much to do for Windows... no sig mask, no thread-level handler table
                TSigHandlerProc handler = itr->first;
                const TSigList& siglist = itr->second;
                for (TSigList::const_iterator sig = siglist.begin(); sig != siglist.end(); ++sig) {
                    signal(*sig, handler);
                }
            }
#endif
        }

        void SetSigMask(int how, sigset_t *mask) const {
            if (how == SIG_SETMASK) {
                Y_VERIFY(!!mask, "Must provide a sig mask");
                SigProcMask(SIG_SETMASK, mask, nullptr);
            } else {
                sigset_t sigmask;
                if (!mask) {
                    SigEmptySet(&sigmask);
                    mask = &sigmask;
                } // else operate on the union of provided mask and my set of signals

                for (TSigSet::const_iterator itr = SigSet.begin(); itr != SigSet.end(); ++itr) {
                    SigAddSet(mask, *itr);
                }
                SigProcMask(how, mask, nullptr);

                if (mask != &sigmask) {
                    SigProcMask(SIG_SETMASK, mask, nullptr); // return current mask to caller
                }
            }
        }

    private:
        TSigHandlerMap SigHandlerMap;
        TSigSet SigSet;
        TSystemEvent Quit;
    };
}
