#pragma once

#include "catalogus.h"
#include "lockablehandle.h"
#include "master_variables.h"
#include "revision.h"
#include "sockaddr.h"
#include "worker_variables.h"

#include <tools/clustermaster/common/log.h>

#include <library/cpp/deprecated/transgene/transgene.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/network/pollerimpl.h>
#include <util/network/sock.h>
#include <util/string/cast.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

enum EWorkerState {
    WS_DISCONNECTED = 0,
    WS_CONNECTING,
    WS_CONNECTED,
    WS_AUTHENTICATING,
    WS_INITIALIZING,
    WS_ACTIVE,
};

template <>
TString ToString<EWorkerState>(const EWorkerState& s);

struct TPollerEvents: TNonCopyable { // TODO better implementation
    typedef TMutex TMyMutex;
    typedef TPollerImpl<TPollerEvents> TPoller;
    typedef TArrayHolder<TPoller::TEvent> TEvents;

    TPoller Poller;
    TEvents Events;

    size_t Reserved;
    size_t Received;

    TPollerEvents()
        : Reserved(0)
        , Received(0)
    {
    }

    void Reserve(size_t count) {
        if (count > Reserved) {
            Reserved = count;
            Events.Reset(new TPoller::TEvent[Reserved]);
        }
    }

    bool Receive(TInstant deadline, TInstant now = TInstant::Now()) noexcept {
        Received = Poller.WaitD(Events.Get(), Reserved, deadline, now);
        return Received;
    }
};

class TMasterGraph;
class TWorkerPool;
class TMasterListManager;

class TWorker: public TNonCopyable {
public:
    TWorker(const TString& hostname, int groupid, TWorkerPool* parent);
    virtual ~TWorker() {}

    void Connect();
    void Disconnect();
    bool CheckConnect();
    void SetFailure(const TString& text) noexcept;

    bool HasDataToSend() const noexcept { return Transceiver.HasDataToSend(); }
    void SendSomeData();
    void RecvSomeData(const TLockableHandle<TMasterGraph>::TWriteLock& graph);

    template <typename TMessageSubclass>
    void EnqueueMessage(const TMessageSubclass& message) {
        DEBUGLOG1(net, Host << ": Enqueuing " << DebugStringForLog(message));
        EnqueueMessageImpl(message.Pack());
    }

    void ProcessMessage(TAutoPtr<NTransgene::TPackedMessage> what, const TLockableHandle<TMasterGraph>::TWriteLock& graph);

private:
    bool Connect(const ISockAddr& sockAddr);
    void EnqueueMessageImpl(TAutoPtr<NTransgene::TPackedMessage> what);

public:
    EWorkerState GetState() const noexcept { return State; }

    const TString& GetHost() const noexcept { return Host; }
    const TString& GetPort() const noexcept { return Port; }
    int GetGroupId() const noexcept { return GroupId; }

    const TNetworkAddress* GetHttpAddress() const noexcept { return HttpAddress.Get() ? &HttpAddress->GetNetworkAddress() : nullptr; }

    TInstant GetConnectTime() const noexcept { return ConnectTime; }
    TInstant GetFailureTime() const noexcept { return FailureTime; }
    TString GetFailureText() const noexcept {
        TGuard<TMutex> guard(FailureTextMutex);
        return FailureText;
    }

    const TWorkerVariablesLight& GetVariables() const noexcept { return VariablesWithRevision.Variables; }
    TRevision::TValue GetVariablesRevision() const noexcept { return VariablesWithRevision.Revision; }

    ui64 GetTotalDiskspace() const noexcept { return TotalDiskspace; }
    i64 GetAvailDiskspace() const noexcept { return AvailDiskspace; }

    const TStreamSocket* GetSocket() const noexcept { return Socket.Get(); }

    TRevision::TValue GetRevision() const noexcept { return Revision; }

private:
    struct TMessageProcessor {
        TWorker*const Worker;
        const TLockableHandle<TMasterGraph>::TWriteLock& Graph;

        TMessageProcessor(TWorker* worker, const TLockableHandle<TMasterGraph>::TWriteLock& graph) noexcept: Worker(worker), Graph(graph) {}
        inline void operator()(TAutoPtr<NTransgene::TPackedMessage> what) const;
    };

    EWorkerState State;

    const TString Host;
    const TString Port;
    const int GroupId;

    THolder<TSocketAddress> HttpAddress;

    ui64 TotalDiskspace;
    i64 AvailDiskspace;

    TInstant ConnectTime;

    TInstant FailureTime;
    TString FailureText;
    TMutex FailureTextMutex; // mutable TString is not thread safe

    class TWorkerVariablesLightWithRevision {
    public:
        void Clear() {
            Variables.Clear();
            Revision.Up();
        }
        void Update(const TVariablesMessage& message) {
            bool changed = Variables.Update(message);
            if (changed)
                Revision.Up();
        }
        TWorkerVariablesLight Variables;
        TRevision Revision;
    };
    TWorkerVariablesLightWithRevision VariablesWithRevision;

    THolder<TStreamSocket> Socket;

    NTransgene::TTransceiver<TMutex> Transceiver;

    TRevision Revision;

    TWorkerPool* const Pool;
};

class WorkersStat {
private:
    int Active;
    int Total;
public:
    WorkersStat(int active, int total)
        : Active(active)
        , Total(total)
    {
    }

    int ActivePercent() {
        if (Total == 0) {
            return 0;
        }
        return ceil(double(Active * 100) / Total);
    }

    int InactiveCount() {
        return Total - Active;
    }
};

class TWorkerPool: public IWorkerPoolVariables, public TNonCopyable {
public:
    typedef TCatalogus<TWorker> TWorkersList;

    TWorkerPool(TMasterListManager *l, TPollerEvents* p);
    ~TWorkerPool() override {}

    void Reset();
    void SetWorkers(const TVector<TString>& workers);

    template <typename TMessageSubclass>
    void EnqueueMessageToWorker(const TString& worker, const TMessageSubclass& what) {
        TWorkersList::const_iterator i = Workers.find(worker);

        if (i != Workers.end() && (*i)->GetState() == WS_ACTIVE)
            (*i)->EnqueueMessage(what);
    }

    template <typename TMessageSubclass>
    void EnqueueMessageAll(const TMessageSubclass& what) {
        for (TWorkersList::const_iterator i = Workers.begin(); i != Workers.end(); ++i)
            if ((*i)->GetState() == WS_ACTIVE)
                (*i)->EnqueueMessage(what);
    }

    void ProcessConnections(TInstant now = TInstant::Now());
    void ProcessEvents(const TLockableHandle<TMasterGraph>::TWriteLock& graph);

    const TWorkerVariablesLight& GetVariablesForWorker(const TString& workername) const override;
    bool CheckIfWorkerIsAvailable(const TString& workername) const override;

    WorkersStat GetWorkersStat() const;

public:
    void PollRead(TWorker* w) const;
    void PollReadWrite(TWorker* w) const;
    void PollNone(TWorker* w) const;

public:
    const TWorkersList& GetWorkers() const noexcept { return Workers; }

private:
    TWorkersList Workers;

    TMasterListManager* const ListManager;
    TPollerEvents* const PollerEvents;
};
