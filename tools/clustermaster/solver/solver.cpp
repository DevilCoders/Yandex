#include "solver.h"

#include "batch.h"
#include "http.h"
#include "request.h"
#include "solve.h"
#include "thingy.h"

#include <tools/clustermaster/communism/core/core.h>
#include <tools/clustermaster/communism/util/daemon.h>
#include <tools/clustermaster/communism/util/dirut.h>
#include <tools/clustermaster/communism/util/file_reopener_by_signal.h>
#include <tools/clustermaster/communism/util/pidfile.h>
#include <tools/clustermaster/proto/solver_options.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/system/daemon.h>
#include <util/system/file.h>
#include <util/system/info.h>
#include <util/system/sigset.h>
#include <util/system/spinlock.h>
#include <util/system/spin_wait.h>
#include <util/thread/pool.h>

#ifdef _unix_
#   include <netinet/tcp.h>

#   ifdef __FreeBSD__
#       ifndef TCP_KEEPIDLE
#           define TCP_KEEPIDLE 0x100
//          warning TCP_KEEPIDLE is not defined. Using compatibility hack, please switch to FreeBSD 9+.
#       endif
#       ifndef TCP_KEEPINTVL
#           define TCP_KEEPINTVL 0x200
//          warning TCP_KEEPINTVL is not defined. Using compatibility hack, please switch to FreeBSD 9+.
#       endif
#   endif
#endif

typedef TLogOutput Log;

namespace NGlobal {

TAtomic NeedSolve = false;
TKeyMapper KeyMapper;
TKnownLimits KnownLimits;
TAtomic LastRequestIndexNumber = 0;
THolder<TFileReopenerBySignal> LogFileReopener;

}

void Run(const NClusterMaster::TSolverOptions&);

void PrintVersionAndExit() {
    Cout << GetProgramSvnVersion() << Endl;
    exit(0);
}

static void Exit0(int) {
    _exit(0);
}

struct TConfigurationError: yexception {};

int main(int argc, char **argv) {

    NGetoptPb::TGetoptPb getoptPb;
    NClusterMaster::TSolverOptions solverOptionsPb;

    try {
        getoptPb.AddOptions(solverOptionsPb);
        getoptPb.GetOpts().AddLongOption('V', "version", "print svn version and exit").NoArgument().Handler(&PrintSvnVersionAndExit0);
        getoptPb.GetOpts().SetFreeArgsMin(0);
        getoptPb.GetOpts().SetFreeArgsMax(500);
        getoptPb.GetOpts().SetFreeArgTitle(0, "KEY=LIMIT");

        TString errorMsg;
        bool ret = getoptPb.ParseArgs(argc, const_cast<const char**>(argv), solverOptionsPb, errorMsg);
        if (ret) {
            Cerr << "Using settings:\n"
                 << "====================\n";
            getoptPb.DumpMsg(solverOptionsPb, Cerr);
            Cerr << "====================\n";
        } else {
            Cerr << errorMsg;
        }

        if (argc <= 1) {
            getoptPb.GetOpts().PrintUsage(argv[0] ? argv[0] : "solver");
            exit(0);
        }

        if (!solverOptionsPb.HasPort() && !solverOptionsPb.HasPath()) {
            throw TConfigurationError() << "nothing to listen";
        }

        if (solverOptionsPb.HasKnownLimitsFile()) {
            TUnbufferedFileInput in(solverOptionsPb.GetKnownLimitsFile());

            LoadMapFromStream(NGlobal::KnownLimits, in);

            for (THashMap<TString, double>::const_iterator i = NGlobal::KnownLimits.begin(); i != NGlobal::KnownLimits.end(); ++i) {
                if (i->second < 0.0) {
                    throw TConfigurationError() << solverOptionsPb.GetKnownLimitsFile() << ": known limit for \"" << i->first << "\" must not be below zero";
                }
            }
        }

        const TVector<TString>& freeArgs = getoptPb.GetOptsParseResult().GetFreeArgs();

        for (TVector<TString>::const_iterator i = freeArgs.begin(); i != freeArgs.end(); ++i) {
            const size_t equal = i->find('=');

            if (equal == TString::npos) {
                throw TConfigurationError() << "invalid known limit definition \"" << *i << "\", the syntax is KEY=LIMIT";
            }

            const TStringBuf key(*i, 0, equal);
            const TStringBuf val(*i, equal + 1, TString::npos);

            double limit = 0.0;

            try {
                limit = FromString<double>(val);
            } catch (const TFromStringException& e) {
                throw TConfigurationError() << "known limit for \"" << key << "\": " << e.what();
            }

            if (limit < 0.0) {
                throw TConfigurationError() << "known limit for \"" << key << "\" must not be below zero";
            }

            NGlobal::KnownLimits[key] = limit;
        }

        InitializeDaemonGeneric(solverOptionsPb.GetLogfile(), solverOptionsPb.GetPidFile(), solverOptionsPb.GetDoNotDaemonize());

#ifdef _unix_
    signal(SIGPIPE, SIG_IGN);

    signal(SIGTERM, Exit0);
    signal(SIGINT, Exit0);

    if (solverOptionsPb.GetDoNotDaemonize())
        signal(SIGHUP, Exit0);
#endif

    } catch (const TConfigurationError& e) {
        Cerr << "Configuration error: " << e.what() << '\n';
        exit(1);
    }

    Run(solverOptionsPb);

    ythrow yexception() << "end of main() scope";
}

struct TLocalListener: TLocalStreamSocket, TPollerCookie {};
struct TInetListener: TInet6StreamSocket, TPollerCookie {};

struct TRequestsComparer {
    bool operator()(const TRequest& first, const TRequest& second) const noexcept {
        return first.Priority > second.Priority || (first.Priority == second.Priority && first.IndexNumber < second.IndexNumber);
    }

    bool operator()(const TRequest* first, const TRequest* second) const noexcept {
        return operator()(*first, *second);
    }
};

void Run(const NClusterMaster::TSolverOptions& options) {
    TRequestsHandle Requests;

    TSolverHttpServer serv(THttpServerOptions(options.GetHTTPPort()), Requests);
    serv.RestartRequestThreads(1, 100);
    serv.Start();

    THolder<TInetListener> InetListener;
    THolder<TLocalListener> LocalListener;

    TBatches Batches;

    NGlobal::TPoller Poller;

    TThreadPool BatchProcessingPool;

    while (true) try {
        // Inet listener
        if (options.HasPort() && !InetListener.Get()) {
            Log() << "info: Creating inet listener"sv;

            THolder<TInetListener> listener(new TInetListener);

            listener->CheckSock();
            SetNonBlock(*listener);

            if (SetSockOpt(*listener, SOL_SOCKET, SO_KEEPALIVE, 1) != 0) {
                Log() << "warning: SetSockOpt(SO_KEEPALIVE) failed"sv;
            }
            if (SetSockOpt(*listener, SOL_SOCKET, SO_REUSEADDR, 1) != 0) {
                Log() << "warning: SetSockOpt(SO_REUSEADDR) failed"sv;
            }

#if defined _unix_ && !defined _darwin_
            if (SetSockOpt(*listener, IPPROTO_TCP, TCP_KEEPIDLE, NCore::KeepaliveIdle.Seconds()) != 0) {
                Log() << "warning: SetSockOpt(TCP_KEEPIDLE) failed"sv;
            }
            if (SetSockOpt(*listener, IPPROTO_TCP, TCP_KEEPINTVL, NCore::KeepaliveIntvl.Seconds()) != 0) {
                Log() << "warning: SetSockOpt(TCP_KEEPINTVL) failed"sv;
            }

#   ifndef __FreeBSD__
// this is buggy on FreeBSD 9.0 (incl. -STABLE)
// remove ifndef (but not the code) when it became fixed (ask marakasov@)
// without this call the default number of 8 probes will be used

            if (SetSockOpt(*listener, IPPROTO_TCP, TCP_KEEPCNT, NCore::KeepaliveCnt) != 0) {
                Log() << "warning: SetSockOpt(TCP_KEEPCNT) failed"sv;
            }
#   endif
#endif

            TSockAddrInet6Stream addr("::", options.GetPort());

            if (SetSockOpt(*listener, IPPROTO_IPV6, IPV6_V6ONLY, 0) != 0) {
                Log() << "warning: SetSockOpt(IPV6_V6ONLY = false) failed"sv;
            }

            int error = 0;
            if ((error = listener->Bind(&addr)) < 0) {
                ythrow TSystemError(-error) << "bind inet listener socket"sv;
            }
            if ((error = listener->Listen(options.GetListenersBacklog())) < 0) {
                ythrow TSystemError(-error) << "listen inet listener socket"sv;
            }

            Poller.Set(listener->GetCookie(), *listener, CONT_POLL_READ);

            InetListener.Reset(listener.Release());

            Log() << "info: Inet listener created: "sv << addr.ToString();
        }

        // Local listener
        if (options.HasPath() && !LocalListener.Get()) {
            Log() << "info: Creating local listener"sv;

            THolder<TLocalListener> listener(new TLocalListener);

            listener->CheckSock();
            SetNonBlock(*listener);

            TSockAddrLocalStream addr(options.GetPath().data());

            int error = 0;
            if ((error = listener->Bind(&addr, 0666)) < 0) {
                ythrow TSystemError(-error) << "bind local listener socket"sv;
            }
            if ((error = listener->Listen(options.GetListenersBacklog())) < 0) {
                ythrow TSystemError(-error) << "listen local listener socket"sv;
            }

            Poller.Set(listener->GetCookie(), *listener, CONT_POLL_READ);

            LocalListener.Reset(listener.Release());

            Log() << "info: Local listener created: "sv << addr.ToString();
        }

        break;
    } catch (const yexception& e) {
        if (TDuration::Seconds(options.GetFailureDelaySeconds()) == TDuration::Max() || !options.GetRetryListen()) {
            throw;
        }

        Log() << "error: "sv << e.what();
        Sleep(TDuration::Seconds(options.GetFailureDelaySeconds()));
    }

    Log() << "info: Starting batch processing thread pool"sv;

    BatchProcessingPool.Start(NSystemInfo::NumberOfCpus());

    // Main loop

    Log() << "info: Entering main loop"sv;

    TTempArray<NGlobal::TPoller::TEvent> Events;
    Y_VERIFY(Events.Size(), "TTempArray allocated zero sized");

    TInstant WaitingDeadline = TInstant::Now() + NCore::ReconnectTimeout + NCore::NetworkLatency;

    TAtomic BatchProcessingCounter = 0;

    while (true) try {
PROFILE_ACQUIRE(PROFILE_EVENTS_WAIT)
        const size_t eventsGot = Poller.WaitD(Events.Data(), Events.Size(), WaitingDeadline);
PROFILE_RELEASE(PROFILE_EVENTS_WAIT)

        if (!eventsGot) {
            WaitingDeadline = TInstant::Max();
        }

        TInstant now = TInstant::Now();

        typedef TMap<TBatch*, int, TLess<TBatch*>, TAllocator> TBatchFilters;
        TBatchFilters batchFilters;

PROFILE_ACQUIRE(PROFILE_EVENTS_PROCESS)
        for (NGlobal::TPoller::TEvent* i = Events.Data(); i != Events.Data() + eventsGot; ++i) {
            TBatch* const batch = dynamic_cast<TBatch*>(reinterpret_cast<TPollerCookie*>(NGlobal::TPoller::ExtractEvent(i)));
            TLocalListener* const localListener = dynamic_cast<TLocalListener*>(reinterpret_cast<TPollerCookie*>(NGlobal::TPoller::ExtractEvent(i)));
            TInetListener* const inetListener = dynamic_cast<TInetListener*>(reinterpret_cast<TPollerCookie*>(NGlobal::TPoller::ExtractEvent(i)));

            if (batch && batch->GetSocket()) {
                batchFilters[batch] |= NGlobal::TPoller::ExtractFilter(i);
            } else if (inetListener && NGlobal::TPoller::ExtractFilter(i) & CONT_POLL_READ) {
                Log() << "info: Accepting inet connection"sv;

                while (true) {
                    TAutoPtr<TStreamSocket> socket(new TInet6StreamSocket);
                    socket->CheckSock();
                    TSockAddrInet6Stream addr;

                    int error = 0;
                    do {
                        error = inetListener->Accept(socket.Get(), &addr);
                    } while (-error == EINTR);

                    if (error) {
                        break;
                    }

                    SetNonBlock(*socket);

                    THolder<TBatch> batch(new TBatch(socket, addr, Requests));

                    batch->Send();
                    batch->UpdatePoller(Poller);

                    Log() << "batch "sv << batch->Id << ": Accepted"sv;

                    Batches.PushBack(batch.Release());
                }
            } else if (localListener && NGlobal::TPoller::ExtractFilter(i) & CONT_POLL_READ) {
                Log() << "info: Accepting local connection"sv;

                while (true) {
                    TAutoPtr<TStreamSocket> socket(new TLocalStreamSocket);
                    socket->CheckSock();
                    TSockAddrLocalStream addr;

                    int error = 0;
                    do {
                        error = localListener->Accept(socket.Get(), &addr);
                    } while (-error == EINTR);

                    if (error) {
                        break;
                    }

                    SetNonBlock(*socket);

                    THolder<TBatch> batch(new TBatch(socket, addr, Requests));

                    batch->Send();
                    batch->UpdatePoller(Poller);

                    Log() << "batch "sv << batch->Id << ": Accepted"sv;

                    Batches.PushBack(batch.Release());
                }
            }
        }
PROFILE_RELEASE(PROFILE_EVENTS_PROCESS)

PROFILE_ACQUIRE(PROFILE_BATCH_PROCESS)
        for (TBatchFilters::const_iterator i = batchFilters.begin(); i != batchFilters.end(); ++i) {
            if (!BatchProcessingPool.Add(new TBatchProcessor(&BatchProcessingCounter, i->first, i->second & CONT_POLL_READ, i->second & CONT_POLL_WRITE, &Poller, now))) {
                ythrow yexception() << "error: cannot enqueue batch processing"sv;
            }
        }

        // Wait until all batch processing jobs finished
        TSpinWait spinWait;
        while (AtomicGet(BatchProcessingCounter) != 0) {
            spinWait.Sleep();
        }
PROFILE_RELEASE(PROFILE_BATCH_PROCESS)

        for (TBatches::iterator i = Batches.Begin(); i != Batches.End();) {
            TBatch* const batch = &*i++;

            if (batch->GetSocket() && NCore::HeartbeatTimeout != TDuration::Max() && batch->LastHeartbeat + NCore::HeartbeatTimeout + NCore::NetworkLatency <= now) {
                Log() << "batch "sv << batch->Id << ": Disconnecting: no heartbeat"sv;
                Poller.Remove(*batch->GetSocket());
                batch->ResetSocket(nullptr);
            }

            if (!batch->GetSocket()) {
                delete batch;
            }
        }

        now = TInstant::Now();

        // Solving
        NCommunism::Solve(Requests, WaitingDeadline, Poller);

PROFILE_STRING("---------------------------------")
PROFILE_OUTPUT(PROFILE_BATCH_PROCESS)
PROFILE_OUTPUT(PROFILE_BATCH_UPDATE)
PROFILE_OUTPUT(PROFILE_CLAIMS_COMBINE)
PROFILE_OUTPUT(PROFILE_CLAIMS_OUTPUT)
PROFILE_OUTPUT(PROFILE_EVENTS_PROCESS)
PROFILE_OUTPUT(PROFILE_EVENTS_WAIT)
PROFILE_OUTPUT(PROFILE_MAIN_SOLVING)
PROFILE_OUTPUT(PROFILE_MAIN_SOLVING_1)
PROFILE_OUTPUT(PROFILE_MAIN_SOLVING_2)
PROFILE_OUTPUT(PROFILE_MAIN_SOLVING_3)
PROFILE_OUTPUT(PROFILE_MAIN_SOLVING_4)
PROFILE_OUTPUT(PROFILE_MAIN_SOLVING_5)
PROFILE_OUTPUT(PROFILE_REQUEST_PARSE)
    } catch (...) {
        if (TDuration::Seconds(options.GetFailureDelaySeconds()) == TDuration::Max()) {
            throw;
        }

        Log() << "error: "sv << CurrentExceptionMessage();
        Sleep(TDuration::Seconds(options.GetFailureDelaySeconds()));
    }
}

void NCommunism::Solve(TRequestsHandle Requests, TInstant &WaitingDeadline, NGlobal::TPoller &Poller) {
    TInstant now = TInstant::Now();

    if (Requests->Requested->Empty()) {
        AtomicSet(NGlobal::NeedSolve, false);
    }

    if (AtomicGet(NGlobal::NeedSolve) && WaitingDeadline == TInstant::Max()) {
PROFILE_ACQUIRE(PROFILE_MAIN_SOLVING)

        Log() << "info: Solving"sv;

        TGuard<TSpinLock> guard(*Requests->Lock);

PROFILE_ACQUIRE(PROFILE_MAIN_SOLVING_1)
        TClaims consumed;
        TStateForecast stateForecast;

        for (TRequests::const_iterator i = Requests->Granted->Begin(); i != Requests->Granted->End(); ++i) {
            consumed.Combine(*i);
            stateForecast.Update(*i, i->DisclaimForecast);
            Log() << "batch "sv << i->Batch->Id << ": "sv << i->Key << " \""sv << i->Label << "\" counted"sv;
        }

PROFILE_RELEASE(PROFILE_MAIN_SOLVING_1)
PROFILE_ACQUIRE(PROFILE_MAIN_SOLVING_2)

        TClaims reserved(consumed);

        unsigned currentPriority = Max<unsigned>();
        TClaims currentPriorityWanted;

        typedef TVector<TRequest*, TAllocator> TRequestedVector;
        TRequestedVector requested;

        for (TRequests::iterator i = Requests->Requested->Begin(); i != Requests->Requested->End(); ++i) {
            requested.push_back(&*i);
        }

        Sort(requested.begin(), requested.end(), TRequestsComparer());

PROFILE_RELEASE(PROFILE_MAIN_SOLVING_2)
        for (TRequestedVector::iterator i = requested.begin(); i != requested.end(); ++i) {
PROFILE_ACQUIRE(PROFILE_MAIN_SOLVING_3)
PROFILE_ACQUIRE(PROFILE_MAIN_SOLVING_4)
            TRequest* const request = *i;

            if (request->Priority != currentPriority) {
                currentPriority = request->Priority;
                if (!currentPriorityWanted.Empty()) {
                    reserved.Combine(currentPriorityWanted);
                    currentPriorityWanted.Clear();
                }
            }

            const TInstant disclaimDeadline = stateForecast.Deduce(*request);
            const TInstant disclaimForecast = request->Duration.ToDeadLine(now);
            const bool inTime = disclaimDeadline != TInstant::Max() && disclaimForecast < disclaimDeadline;

            TClaims potential(inTime ? consumed : reserved);

PROFILE_ACQUIRE(PROFILE_MAIN_SOLVING_5)
            if (potential.Combine(*request)) {
                TAutoPtr<NCore::TPackedMessage> message(NCore::TGrantMessage(request->GrantActionId, request->Key).Pack());

                request->State = TRequest::GRANTED;
                request->LinkBefore(&*Requests->Granted->End());

                if (inTime) {
                    DoSwap(consumed, potential);
                    potential.Combine(*request);
                    request->DisclaimForecast = disclaimDeadline;
                } else {
                    DoSwap(reserved, potential);
                    consumed.Combine(*request);
                    request->DisclaimForecast = disclaimForecast;
                    stateForecast.Update(*request, disclaimForecast);
                }

                request->Batch->Enqueue(message);
                request->Batch->UpdatePoller(Poller);

                Log() << "batch "sv << request->Batch->Id << ": "sv << request->Key << " \""sv << request->Label << "\" granted"sv << (inTime ? TStringBuf(" *") : TStringBuf());
            } else {
                currentPriorityWanted.Combine(*request);
            }
PROFILE_RELEASE(PROFILE_MAIN_SOLVING_5)
PROFILE_RELEASE(PROFILE_MAIN_SOLVING_4)
        }

        AtomicSet(NGlobal::NeedSolve, false);

        Log() << "info: Solved ("sv << consumed << ')';
    }
}
