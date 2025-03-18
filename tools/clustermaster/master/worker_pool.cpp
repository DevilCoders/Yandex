#include "worker_pool.h"

#include "log.h"
#include "master.h"
#include "master_profiler.h"
#include "master_target_graph.h"
#include "messages.h"

#include <sys/types.h>

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/yexception.h>
#include <util/network/address.h>
#include <util/network/socket.h>
#include <util/string/split.h>
#include <util/string/util.h>
#include <util/system/error.h>

// #define IS_STERILE

template<>
TString ToString<EWorkerState>(const EWorkerState& s) {
    switch (s) {
    case WS_DISCONNECTED:   return "disconnected";
    case WS_CONNECTING:     return "connecting";
    case WS_CONNECTED:      return "connected";
    case WS_AUTHENTICATING: return "authenticating";
    case WS_INITIALIZING:   return "initializing";
    case WS_ACTIVE:         return "active";
    default: ythrow yexception() << "bad worker state";
    }
}

TWorker::TWorker(const TString& hostname, int groupid, TWorkerPool* parent)
    : State(WS_DISCONNECTED)
    , Host(hostname)
    , Port(Sprintf("%d", MasterOptions.WorkerPort))
    , GroupId(groupid)
    , TotalDiskspace(0)
    , AvailDiskspace(0)
    , Transceiver(ProtoVersion)
    , Pool(parent)
{
    if (hostname.empty())
        ythrow yexception() << "attempt to create worker with empty hostname";
}

bool TWorker::Connect(const ISockAddr& sockAddr) {
    if (!Socket.Get()) {
        return false;
    }
    Socket->CheckSock();
    SetKeepAlive(*Socket, true);
    SetNonBlock(*Socket);

    int error = Socket->Connect(&sockAddr);

    if (error < 0 && -error != EINPROGRESS && -error != EINTR) {
        SetFailure(TString("couldn't connect: ") + LastSystemErrorText(error));
        Disconnect();
        return false;
    }

    Pool->PollReadWrite(this);

    State = WS_CONNECTING;

    ConnectTime = TInstant::Now();

    Revision.Up();
    return true;
}

void TWorker::Connect() {
#ifndef IS_STERILE
    Disconnect();

    THolder<TSocketAddress> workerMyAddrPtr;
    try {
        workerMyAddrPtr.Reset(new TSocketAddress(Host, FromString(Port)));
    } catch (const TNetworkResolutionError& e) {
        SetFailure(TString("couldn't resolve host '") + Host + "': " + e.what());
        Disconnect();
        return;
    }

    THolder<ISockAddr> sockAddr;
    const TNetworkAddress& workerAddr = workerMyAddrPtr->GetNetworkAddress();

    for (TNetworkAddress::TIterator it = workerAddr.Begin(); it != workerAddr.End(); ++it) {
        if (it->ai_family == AF_INET) {
            sockAddr.Reset(new TSockAddrInet());
            memcpy(sockAddr->SockAddr(), it->ai_addr, it->ai_addrlen);
            Socket.Reset(new TInetStreamSocket());
            if (Connect(*sockAddr)) {
                return;
            }
        } else if (it->ai_family == AF_INET6) {
            sockAddr.Reset(new TSockAddrInet6());
            memcpy(sockAddr->SockAddr(), it->ai_addr, it->ai_addrlen);
            Socket.Reset(new TInet6StreamSocket());
            if (Connect(*sockAddr)) {
                return;
            }
        }
    }

    SetFailure(TString("couldn't find appropriate address to connect to '") + Host + "'");
    Disconnect();
#endif
}

void TWorker::Disconnect() {
    if (Socket.Get()) {
        Pool->PollNone(this);

        Transceiver.ResetState();
        Transceiver.ResetQueue();

        Socket.Destroy();

        HttpAddress.Destroy();

        State = WS_DISCONNECTED;

        Revision.Up();
        VariablesWithRevision.Revision.Up();
    }
}

bool TWorker::CheckConnect() {
    // select() indicated writability on a connection socket -> it was connected
    int error = 0;

    if (GetSockOpt(*Socket, SOL_SOCKET, SO_ERROR, error) == -1)
        ythrow yexception() << "getsockopt: " <<  LastSystemErrorText();

    if (error != 0) {
        SetFailure(TString("couldn't connect: ") + LastSystemErrorText(error));
        Disconnect();
        return false;
    }

    State = WS_CONNECTED;

    Revision.Up();

    return true;
}

void TWorker::SetFailure(const TString& text) noexcept {
    TGuard<TMutex> guard(FailureTextMutex);

    FailureTime = TInstant::Now();
    FailureText = text;

    Revision.Up();
}

void TWorker::SendSomeData() {
    Transceiver.Send(Socket.Get());
}

void TWorker::RecvSomeData(const TLockableHandle<TMasterGraph>::TWriteLock& graph) {
    Transceiver.Recv<const TMessageProcessor>(Socket.Get(), TMessageProcessor(this, graph));
}

void TWorker::EnqueueMessageImpl(TAutoPtr<NTransgene::TPackedMessage> what) {
    GetMessageProfiler2().Sent(static_cast<NProto::EMessageType>(what->GetType()));
    Transceiver.Enqueue(what);
    Pool->PollReadWrite(this);
}

void TWorker::ProcessMessage(TAutoPtr<NTransgene::TPackedMessage> what, const TLockableHandle<TMasterGraph>::TWriteLock& graph) {
    PROFILE_ACQUIRE(ID_MESSAGES_ALL_TIME)
    switch (what->GetType()) {
    case TCommandMessage::Type:
        {
            /*
             * WARN: Command propagating is used for cycle realization only at the moment. It's the brute hack so
             * you need to pay attention to if propagating will be used for other purposes
             */
            TCommandMessage in(*what);
            LOG("Master got command for propagating: " << in.GetTarget() << ", " << in.GetFlags());

            if (in.GetFlags() == TCommandMessage::CF_INVALIDATE && in.GetState() == TS_SUCCESS) {
                graph->PropagateThinStatus(in.GetTarget(), TS_IDLE, Pool);
            }
            Pool->EnqueueMessageAll(in);
        }
        break;
    case TErrorMessage::Type:
        {
            TErrorMessage in(*what);

            if (in.GetFatal())
                ythrow yexception() << "Fatal error from worker: " << in.GetError();
            else
                SetFailure(in.GetError());
        }
        break;
    case TWorkerHelloMessage::Type:
        {
            if (State != WS_CONNECTED) {
                LOG1(net, Host << ": Unexpected hello message received, ignoring");
                return;
            }

            LOG1(net, Host << ": Hello message received, verifying worker version");

            TWorkerHelloMessage in(*what);

            HttpAddress.Reset(new TSocketAddress(Host, in.GetHttpPort()));

            // Send auth reply
            LOG1(net, Host << ": Worker version ok, sending auth reply");

            TAuthReplyMessage message;

            in.GenerateResponse(*message.MutableDigest(), MasterOptions.AuthKeyPath);

            EnqueueMessage(message);

            State = WS_AUTHENTICATING;

            Revision.Up();
        }
        break;
    case TAuthSuccessMessage::Type:
        {
            if (State != WS_AUTHENTICATING) {
                LOG1(net, Host << ": Unexpected authorification success notification received, ignoring");
                return;
            }

            LOG1(net, Host << ": Authorification success notification received, sending config");

            // reply with new config
            TConfigMessage msg;

            graph->ExportConfig(Host, &msg);

            EnqueueMessage(msg);

            State = WS_INITIALIZING;

            Revision.Up();
        }
        break;
    case TFullStatusMessage::Type:
        {
            if (State < WS_INITIALIZING) {
                LOG1(net, Host << ": Unexpected full status received, ignoring");
                return;
            }

            TFullStatusMessage in(*what);

            LOG1(net, Host << ": Full status received, processing, size=" << what->GetSize());

            graph->ProcessFullStatus(in, Host, Pool);

            State = WS_ACTIVE;

            graph->MaintainReloadScriptState(Pool->GetWorkersStat());

            Revision.Up();
        }
        break;
    case TSingleStatusMessage::Type:
        {
            if (State < WS_ACTIVE) {
                LOG1(net, Host << ": Unexpected single status received, ignoring");
                return;
            }

            TSingleStatusMessage in(*what);

            DEBUGLOG1(net, Host << ": received " << DebugStringForLog(in));

            graph->ProcessSingleStatus(in, Host, Pool);

            Revision.Up();
        }
        break;
    case TThinStatusMessage::Type:
        {
            if (State < WS_ACTIVE) {
                LOG1(net, Host << ": Unexpected thin status received, ignoring");
                return;
            }

            TThinStatusMessage in(*what);

            if (false) {
                LOG1(net, Host << ": Thin status received, processing, size=" << what->GetSize());
            }

            graph->ProcessThinStatus(in, Host, Pool);

            Revision.Up();
        }
        break;
    case TVariablesMessage::Type:
        {
            if (State < WS_INITIALIZING) {
                LOG1(net, Host << ": Unexpected variables message received, ignoring");
                return;
            }

            TVariablesMessage in(*what);

            LOG1(net, Host << ": Variables message (" << in.FormatText() << ") received, processing");

            if (State < WS_ACTIVE)
                VariablesWithRevision.Clear();

            VariablesWithRevision.Update(in);
        }
        break;
    case TDiskspaceMessage::Type:
        {
            if (State < WS_INITIALIZING) {
                LOG1(net, Host << ": Unexpected disksize message received, ignoring");
                return;
            }

            TDiskspaceMessage in(*what);

            TotalDiskspace = in.GetTotal();
            AvailDiskspace = in.GetAvail();

            Revision.Up();
        }
        break;
    default:
        ythrow yexception() << "unknown message type received: " << (int)what->GetType();
    }
    PROFILE_RELEASE(ID_MESSAGES_ALL_TIME)
}

inline void TWorker::TMessageProcessor::operator()(TAutoPtr<NTransgene::TPackedMessage> what) const {
    GetMessageProfiler2().Received((NProto::EMessageType) what->GetType());
    Worker->ProcessMessage(what, Graph);
}

TWorkerPool::TWorkerPool(TMasterListManager *l, TPollerEvents* p)
    : ListManager(l)
    , PollerEvents(p)
{
}

void TWorkerPool::Reset() {
    for (TWorkersList::const_iterator i = Workers.begin(); i != Workers.end(); ++i)
        (*i)->Disconnect();

    Workers.reset();
}

void TWorkerPool::SetWorkers(const TVector<TString>& workers) {
    LOG("Setting workers");

    Reset();

    PollerEvents->Reserve(workers.size() * 2);

    for (TVector<TString>::const_iterator i = workers.begin(); i != workers.end(); ++i)
        if (Workers.find(*i) == Workers.end())
            Workers.push_back(new TWorker(*i, ListManager->GetGroupId(*i), this), *i);
}

void TWorkerPool::ProcessConnections(TInstant now) {
    if (Workers.size() != 0) {
        for (TWorkersList::const_iterator i = Workers.begin(); i != Workers.end(); ++i) {
            if (((*i)->GetState() == WS_DISCONNECTED && now - (*i)->GetFailureTime() > TDuration::Seconds(MasterOptions.NetworkRetry)) ||
                ((*i)->GetState() == WS_CONNECTING && now - (*i)->GetConnectTime() > TDuration::Seconds(MasterOptions.NetworkRetry)))
            {
                (*i)->Connect();
            }
        }
    } else {
        // If no sleep here and there are no workers (empty script) master eats 100% cpu cycling in main loop
        sleep(MasterOptions.NetworkHeartbeat);
    }
}

void TWorkerPool::ProcessEvents(const TLockableHandle<TMasterGraph>::TWriteLock& graph) {
    for (TPollerEvents::TPoller::TEvent* i = PollerEvents->Events.Get(); i != PollerEvents->Events.Get() + PollerEvents->Received; ++i) {
        TWorker* worker = reinterpret_cast<TWorker*>(TPollerEvents::TPoller::ExtractEvent(&*i));

        if (worker->GetState() > WS_DISCONNECTED) try {
            if (TPollerEvents::TPoller::ExtractStatus(&*i) == EIO)
                if (worker->GetState() >= WS_CONNECTED) {
                    ythrow yexception() << "host disconnected";
                } else {
                    worker->SetFailure("Erroneous socket status");
                    worker->Disconnect();
                    continue;
                }

            if (TPollerEvents::TPoller::ExtractFilter(&*i) & CONT_POLL_WRITE) {
                // if we're failed to connect, break
                if (worker->GetState() == WS_CONNECTING && !worker->CheckConnect())
                    continue;

                // send some data if everything's ok
                if (worker->GetState() >= WS_CONNECTED)
                    worker->SendSomeData();
            }
            if (TPollerEvents::TPoller::ExtractFilter(&*i) & CONT_POLL_READ) {
                // recv some data if everything's ok
                if (worker->GetState() >= WS_CONNECTED)
                    worker->RecvSomeData(graph);
            }

            if (worker->HasDataToSend())
                PollReadWrite(worker);
            else
                PollRead(worker);
        } catch (const NTransgene::TDisconnection& /*e*/) {
            ERRORLOG1(net, worker->GetHost() << ": Worker disconnected");
            worker->SetFailure(TString());
            worker->Disconnect();
        } catch (const NTransgene::TBadVersion& e) {
            ERRORLOG1(net, worker->GetHost() << ": Bad worker protocol version: " << e.what());
            worker->SetFailure(TString("Bad worker protocol version: ") + e.what());
            worker->Disconnect();
        } catch (...) {
            const auto currentExceptionMessage = CurrentExceptionMessage();
            ERRORLOG1(net, worker->GetHost() << ": Disconnecting worker, reason: " << currentExceptionMessage);
            Cerr << currentExceptionMessage;
            worker->SetFailure(currentExceptionMessage);
            worker->Disconnect();
        }
    }
}

void TWorkerPool::PollRead(TWorker* w) const {
    PollerEvents->Poller.Set(w, *w->GetSocket(), CONT_POLL_READ);
}

void TWorkerPool::PollReadWrite(TWorker* w) const {
    PollerEvents->Poller.Set(w, *w->GetSocket(), CONT_POLL_READ | CONT_POLL_WRITE);
}

void TWorkerPool::PollNone(TWorker* w) const {
    PollerEvents->Poller.Remove(*w->GetSocket());
}

const TWorkerVariablesLight& TWorkerPool::GetVariablesForWorker(const TString& workername) const {
    TWorkersList::const_iterator worker = Workers.find(workername);
    if (worker == Workers.end())
        ythrow yexception() << "getting variables for unknown worker";

    return (*worker)->GetVariables();
}

bool TWorkerPool::CheckIfWorkerIsAvailable(const TString& workername) const {
    TWorkersList::const_iterator worker = Workers.find(workername);
    if (worker == Workers.end())
        ythrow yexception() << "checking the unknown worker";

    return (*worker)->GetState() == WS_ACTIVE;
}

WorkersStat TWorkerPool::GetWorkersStat() const {
    int total = Workers.size();
    int active = 0;
    for (TWorkersList::const_iterator i = Workers.begin(); i != Workers.end(); ++i) {
        if ((*i)->GetState() == WS_ACTIVE) {
            active++;
        }
    }
    return WorkersStat(active, total);
}

