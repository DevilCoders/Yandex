#pragma once

#include <tools/clustermaster/communism/core/core.h>
#include <tools/clustermaster/communism/core/definition.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <util/generic/yexception.h>
#include <util/network/pollerimpl.h>
#include <util/network/sock.h>
#include <util/system/event.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>
#include <util/system/tls.h>

#include <utility>

namespace NCommunism {

struct TNotDefined: yexception {};
struct TDefined: yexception {};

struct TDefinition: NProto::TDefinition {
    typedef NProto::TDefinition::TClaim TClaim;
    typedef google::protobuf::RepeatedPtrField<TClaim> TRepeatedClaim;

    TDefinition& AddClaim(const TString& key, double val) {
        Y_ASSERT(!key.empty() && val <= 1.0);
        TClaim* const claim = NProto::TDefinition::AddClaim();
        claim->SetKey(key);
        claim->SetVal(val);
        return *this;
    }

    TDefinition& AddSharedClaim(const TString& key, double val, const TString& name) {
        Y_ASSERT(!key.empty() && val <= 1.0 && !name.empty());
        TClaim* const claim = NProto::TDefinition::AddClaim();
        claim->SetKey(key);
        claim->SetVal(val);
        claim->SetName(name);
        return *this;
    }
};

struct TDetails: NProto::TDetails {
    TDetails& SetPriority(double priority) {
        Y_ASSERT(priority >= 0.0 && priority <= 1.0);
        NProto::TDetails::SetPriority(priority);
        return *this;
    }

    TDetails& SetDuration(TDuration duration) {
        NProto::TDetails::SetDuration(duration.Seconds());
        return *this;
    }

    TDetails& SetLabel(const TString& label) {
        NProto::TDetails::SetLabel(label);
        return *this;
    }

    bool Empty() const noexcept {
        return !HasPriority() && !HasDuration() && !HasLabel();
    }
};

enum EStatus {
    BADVERSION,
    CONNECTED,
    RECONNECTING,

    GRANTED,
    EXPIRED,
    REJECTED,
};

struct TGroupingOps {
    template <class TClient>
    static void Acquire(TClient* client) noexcept try {
        client->StartGrouping();
    } catch (...) {
    }

    template <class TClient>
    static void Release(TClient* client) noexcept try {
        client->FinishGrouping();
    } catch (...) {
    }
};

struct TDefThreadIntEvent: TSystemEvent {
    TDefThreadIntEvent()
        : TSystemEvent(TSystemEvent::rManual)
    {
    }
};

template <class TUserdata, class TThreadIntEvent = TDefThreadIntEvent>
class TClient: private TThread {
public:
    struct TEvent {
        TUserdata Userdata;
        EStatus Status;
        TString Message;

        TEvent(typename TTypeTraits<TUserdata>::TFuncParam userdata, EStatus status, const TString& message)
            : Userdata(userdata)
            , Status(status)
            , Message(message)
        {
        }
    };

    typedef TList<TEvent> TEvents;

    TClient();
    ~TClient();

    inline void Start();
    inline void Reset();

    inline void Define(typename TTypeTraits<TUserdata>::TFuncParam userdata, const ISockAddr& solver, const TDefinition& definition);
    inline void Define(typename TTypeTraits<TUserdata>::TFuncParam userdata, typename TTypeTraits<TUserdata>::TFuncParam origin);
    inline void Undef(typename TTypeTraits<TUserdata>::TFuncParam userdata);

    inline void Details(typename TTypeTraits<TUserdata>::TFuncParam userdata, const TDetails& details);

    inline void Request(typename TTypeTraits<TUserdata>::TFuncParam userdata, TInstant deadline);

    inline void Request(typename TTypeTraits<TUserdata>::TFuncParam userdata, TDuration timeout) {
        Request(userdata, timeout.ToDeadLine());
    }

    inline void Request(typename TTypeTraits<TUserdata>::TFuncParam userdata) {
        Request(userdata, TInstant::Max());
    }

    inline void Claim(typename TTypeTraits<TUserdata>::TFuncParam userdata, bool asynchronous);
    inline void Disclaim(typename TTypeTraits<TUserdata>::TFuncParam userdata);

    // Use this feature if only you really need it. Do not continue grouping for a considerable time.
    inline void StartGrouping();
    inline void FinishGrouping();

    inline void PullEvents(TEvents& events);
    inline void PullEvents(TEvents& events, typename TTypeTraits<TUserdata>::TFuncParam userdata);

    inline bool WaitD(TInstant deadline);

    inline bool WaitT(TDuration timeout) {
        return WaitD(timeout.ToDeadLine());
    }

    inline void WaitI() {
        WaitD(TInstant::Max());
    }

    inline void SetCustomEvent(const TEvent& event);

    inline void Signal();

    inline TAutoPtr< TVector<SOCKET> > GetSockets() const;

private:
    class TImpl;
    TIntrusivePtr<TImpl> Impl;
};

inline TIpPort GetDefaultPort() noexcept {
    return NCore::DefaultPort;
}

inline TDuration GetHeartbeatTimeout() noexcept {
    return NCore::HeartbeatTimeout;
}

typedef NCore::TPackedMessage TNetworkMessage;

inline TAutoPtr<TNetworkMessage> GetHeartbeatMessage() {
    return NCore::THeartbeatMessage().Pack();
}

// Implementation

template <class T, class TThreadIntEvent>
class TClient<T, TThreadIntEvent>::TImpl: public TAtomicRefCount<TImpl> {
public:
    TThreadIntEvent ThreadEvent;

    TMutex Mutex;

    NTls::TValue<bool> Grouping;

    struct TBatch;

    struct TUpdater {
        TImpl* const Client;
        TBatch* const Batch;

        TUpdater(TImpl* client, TBatch* batch)
            : Client(client)
            , Batch(batch)
        {
        }

        void operator ()(TAutoPtr<NCore::TPackedMessage> what);
    };

    struct TBatch: NCore::TTransceiver<TUpdater>, TSimpleRefCount<TBatch> {
        THolder<ISockAddr> Solver;
        TInstant LastHeartbeat;
        TInstant LastReconnect;

        NTls::TValue<NCore::TPackedMessageList> Group;

        TBatch(TImpl* client, const ISockAddr& solver)
            : NCore::TTransceiver<TUpdater>(TUpdater(client, this), nullptr, Max<size_t>())
            , LastHeartbeat(TInstant::Zero())
            , LastReconnect(TInstant::Now())
        {
            const TSockAddrInet*  const solverInet  = dynamic_cast<const TSockAddrInet*>(&solver);
            const TSockAddrInet6* const solverInet6 = dynamic_cast<const TSockAddrInet6*>(&solver);
            const TSockAddrLocal* const solverLocal = dynamic_cast<const TSockAddrLocal*>(&solver);

            if (solverInet) {
                Solver.Reset(new TSockAddrInet(*solverInet));
            } else if (solverInet6) {
                Solver.Reset(new TSockAddrInet6(*solverInet6));
            } else if (solverLocal) {
                Solver.Reset(new TSockAddrLocal(*solverLocal));
            } else {
                ythrow yexception() << "specified ISockAddr is not in [TSockAddrInet, TSockAddrInet6, TSockAddrLocal]"sv;
            }

            Connect();
        }

        void Connect();

        template <class TLockPolicy>
        void UpdatePoller(TPollerImpl<TLockPolicy>& poller) const noexcept {
            poller.Set(const_cast<void*>(reinterpret_cast<const void*>(this)), *this->GetSocket(), CONT_POLL_READ | (this->HasDataToSend() ? CONT_POLL_WRITE : 0));
        }

        void EnqueueOrGroup(TAutoPtr<NCore::TPackedMessage> message, bool group) noexcept {
            if (!group) {
                this->Enqueue(message);
            } else {
                Group.Get().PushBack(message.Release());
            }
        }
    };

    typedef TIntrusivePtr<TBatch> TBatchHandle;
    struct TBatchHandleHash;
    struct TBatchHandleEqualTo;
    typedef THashSet<TBatchHandle, TBatchHandleHash, TBatchHandleEqualTo> TBatches;

    struct TSameSolverPredicate;

    struct TRequest {
        const NCore::TKey Key;
        const T Userdata;

        TBatchHandle Batch;
        TSimpleSharedPtr<TDefinition> Definition;
        TDetails Details;

        enum EState {
            S_DEFINED,
            S_REQUESTED,
            S_GRANTED,
        };

        EState State;

        NCore::TActionId RejectActionId;
        NCore::TActionId GrantActionId;

        TInstant Deadline;

        struct TExtractKey {
            NCore::TKey operator()(const TRequest& from) const noexcept {
                return from.Key;
            }
        };

        struct TLessDeadline: private TLess<TInstant::TValue> {
            bool operator()(const TRequest* first, const TRequest* second) const {
                return TLess<TInstant::TValue>::operator()(first->Deadline.GetValue(), second->Deadline.GetValue());
            }
            using is_transparent = void;
        };

        TRequest(NCore::TKey key, typename TTypeTraits<T>::TFuncParam userdata)
            : Key(key)
            , Userdata(userdata)
            , State(S_DEFINED)
            , RejectActionId(0)
            , GrantActionId(0)
        {
        }
    };

    struct TRequests : THashTable<TRequest, NCore::TKey, THash<NCore::TKey>, typename TRequest::TExtractKey, TEqualTo<NCore::TKey>, std::allocator<TRequest>>, TNonCopyable {
        enum {
            MaxUserdataCollisions = 32
        };

        TRequests()
            : THashTable<typename TRequests::value_type, typename TRequests::key_type, typename TRequests::hasher, typename TRequest::TExtractKey, typename TRequests::key_equal, std::allocator<typename TRequests::value_type>>(100, typename TRequests::hasher(), typename TRequests::key_equal())
        {
        }

        typename TRequests::iterator FindByUserdata(typename TTypeTraits<T>::TFuncParam userdata) {
            typename TRequests::iterator request = TRequests::find(static_cast<NCore::TKey>(THash<T>()(userdata)));
            unsigned attempt = MaxUserdataCollisions;

            while (request != TRequests::end() && !TEqualTo<T>()(request->Userdata, userdata) && --attempt) {
                request = TRequests::find(IntHash(request->Key));
            }

            if (!attempt) {
                return TRequests::end();
            }

            return request;
        }

        std::pair<typename TRequests::iterator, bool> InsertByUserdata(typename TTypeTraits<T>::TFuncParam userdata) {
            std::pair<typename TRequests::iterator, bool> request = TRequests::insert_unique(TRequest(static_cast<NCore::TKey>(THash<T>()(userdata)), userdata));
            unsigned attempt = MaxUserdataCollisions;

            while (!TEqualTo<T>()(request.first->Userdata, userdata) && --attempt) {
                request = TRequests::insert_unique(TRequest(IntHash(request.first->Key), userdata));
            }

            if (!attempt) {
                ythrow yexception() << "no room for request with this userdata"sv;
            }

            return request;
        }
    };

    struct TDeadlines: TMultiSet<TRequest*, typename TRequest::TLessDeadline> {
        typedef TMultiSet<TRequest*, typename TRequest::TLessDeadline> TBase;

        void EraseByPointer(const TRequest* request) {
            const std::pair<typename TDeadlines::iterator, typename TDeadlines::iterator> range = TBase::equal_range(request);
            for (typename TDeadlines::iterator i = range.first; i != range.second; ++i) {
                if (*i == request) {
                    TBase::erase(i);
                    break;
                }
            }
        }
    };

    struct TExtractUserdataFromEvent {
        const T& operator()(const TEvent& from) const noexcept {
            return from.Userdata;
        }
    };

    struct TEventsMap : THashTable<TEvent, T, THash<T>, TExtractUserdataFromEvent, TEqualTo<T>, std::allocator<TEvent>> {
        typedef THashTable<typename TEventsMap::value_type, typename TEventsMap::key_type, typename TEventsMap::hasher, TExtractUserdataFromEvent, typename TEventsMap::key_equal, std::allocator<typename TEventsMap::value_type>> TBase;

        TEventsMap()
            : TBase(7, typename TBase::hasher(), typename TBase::key_equal())
        {
        }

        void Set(const TEvent& event) {
            const std::pair<typename TBase::iterator, bool> ret = TBase::insert_unique(event);
            if (!ret.second) {
                *ret.first = event;
            }
        }
    };

    TBatches Batches;
    TRequests Requests;
    TDeadlines Deadlines;

    TEventsMap BatchEvents;
    TEventsMap RequestEvents;

    typedef TPollerImpl<TWithoutLocking> TPoller;

    TPoller Poller;
    TTempArray<TPoller::TEvent> PollerEvents;

    TImpl() {
        Y_VERIFY(PollerEvents.Size(), "TTempArray allocated zero sized");
    }

    static void* ClientThreadProc(void* param) noexcept {
        try {
            TIntrusivePtr<TImpl>(reinterpret_cast<TClient*>(param)->Impl)->Run();
            return nullptr;
        } catch (const yexception& e) {
            Y_FAIL("error: %s", e.what());
            return nullptr;
        } catch (const std::bad_alloc& e) {
            Y_FAIL("error (std::bad_alloc): %s", e.what());
            return nullptr;
        } catch (...) {
            Y_FAIL("error (unknown exception): %s", CurrentExceptionMessage().data());
            return nullptr;
        }
    }

    void Run();

    void Define(typename TTypeTraits<T>::TFuncParam userdata, const ISockAddr& solver, const TDefinition& definition);
    void Define(typename TTypeTraits<T>::TFuncParam userdata, typename TTypeTraits<T>::TFuncParam origin);
    void Undef(typename TTypeTraits<T>::TFuncParam userdata);

    void Details(typename TTypeTraits<T>::TFuncParam userdata, const TDetails& details);

    void Request(typename TTypeTraits<T>::TFuncParam userdata, TInstant deadline);
    void Claim(typename TTypeTraits<T>::TFuncParam userdata, bool asynchronous);
    void Disclaim(typename TTypeTraits<T>::TFuncParam userdata);

    inline void StartGrouping();
    inline void FinishGrouping();

    inline void PullEvents(TEvents& events);
    inline void PullEvents(TEvents& events, typename TTypeTraits<T>::TFuncParam userdata);

    inline void SetCustomEvent(const TEvent& event);

    inline TAutoPtr< TVector<SOCKET> > GetSockets() const;
};

template <class T, class E>
void TClient<T, E>::TImpl::TUpdater::operator()(TAutoPtr<NCore::TPackedMessage> what) {
    TGuard<TMutex> guard(Client->Mutex);

    switch (what->GetType()) {
    case NCore::TGrantMessage::Type: {
        const NCore::TGrantMessage message(*what);

        typename TClient::TImpl::TRequests::iterator request = Client->Requests.find(message.GetKey());

        if (request != Client->Requests.end() && request->Batch == Batch && request->GrantActionId == message.GetActionId() && request->State != TRequest::S_DEFINED) {
            if (request->State == TRequest::S_REQUESTED) {
                Client->Deadlines.EraseByPointer(&*request);
            }

            request->State = TRequest::S_GRANTED;

            Client->RequestEvents.Set(TEvent(request->Userdata, GRANTED, TString()));
            Client->ThreadEvent.Signal();
        }

        break;
    } case NCore::TRejectMessage::Type: {
        const NCore::TRejectMessage message(*what);

        typename TClient::TImpl::TRequests::iterator request = Client->Requests.find(message.GetKey());

        if (request != Client->Requests.end() && request->Batch == Batch && request->RejectActionId == message.GetActionId()) {
            if (request->State == TRequest::S_REQUESTED) {
                Client->Deadlines.EraseByPointer(&*request);
            }

            Client->RequestEvents.Set(TEvent(request->Userdata, REJECTED, message.GetMessage()));
            Client->BatchEvents.erase(request->Userdata);

            Client->Requests.erase(request);

            Client->ThreadEvent.Signal();
        }

        break;
    }
    }
}

template <class T, class E>
void TClient<T, E>::TImpl::TBatch::Connect() {
    if (dynamic_cast<const TSockAddrInet*>(Solver.Get())) {
        THolder<TInetStreamSocket> socket(new TInetStreamSocket());
        socket->CheckSock();
        SetNonBlock(*socket);

        while (socket->Connect(Solver.Get()) == -EINTR)
            {}

        TBatch::ResetSocket(socket.Release());
    } else if (dynamic_cast<const TSockAddrInet6*>(Solver.Get())) {
        THolder<TInet6StreamSocket> socket(new TInet6StreamSocket());
        socket->CheckSock();
        SetNonBlock(*socket);

        while (socket->Connect(Solver.Get()) == -EINTR)
            {}

        TBatch::ResetSocket(socket.Release());
    } else if (dynamic_cast<const TSockAddrLocal*>(Solver.Get())) {
        THolder<TLocalStreamSocket> socket(new TLocalStreamSocket());
        socket->CheckSock();
        SetNonBlock(*socket);

        while (socket->Connect(Solver.Get()) == -EINTR)
            {}

        TBatch::ResetSocket(socket.Release());
    } else {
        ythrow yexception() << "bad solver ISockAddr"sv;
    }
}

template <class T, class E>
struct TClient<T, E>::TImpl::TBatchHandleHash: private THash<void*> {
    size_t operator()(const TBatchHandle& what) const {
        return THash<void*>::operator()(reinterpret_cast<void*>(what.Get()));
    }
};

template <class T, class E>
struct TClient<T, E>::TImpl::TBatchHandleEqualTo: private TEqualTo<void*> {
    bool operator()(const TBatchHandle& first, const TBatchHandle& second) const {
        return TEqualTo<void*>::operator()(reinterpret_cast<void*>(first.Get()), reinterpret_cast<void*>(second.Get()));
    }
};

template <class T, class E>
struct TClient<T, E>::TImpl::TSameSolverPredicate {
    const TSockAddrInet* const SolverInet;
    const TSockAddrInet6* const SolverInet6;
    const TSockAddrLocal* const SolverLocal;

    TSameSolverPredicate(const ISockAddr& solver)
        : SolverInet(dynamic_cast<const TSockAddrInet*>(&solver))
        , SolverInet6(dynamic_cast<const TSockAddrInet6*>(&solver))
        , SolverLocal(dynamic_cast<const TSockAddrLocal*>(&solver))
    {
        if (!SolverInet && !SolverInet6 && !SolverLocal) {
            ythrow yexception() << "bad solver ISockAddr"sv;
        }
    }

    bool operator()(const TBatchHandle& batch) const {
        if (!batch.Get()) {
            return false;
        } else if (SolverInet) {
            const TSockAddrInet* const solver = dynamic_cast<const TSockAddrInet*>(batch->Solver.Get());
            return solver && solver->GetIp() == SolverInet->GetIp() && solver->GetPort() == SolverInet->GetPort();
        } else if (SolverInet6) {
            const TSockAddrInet6* const solver = dynamic_cast<const TSockAddrInet6*>(batch->Solver.Get());
            return solver && solver->GetIp() == SolverInet6->GetIp() && solver->GetPort() == SolverInet6->GetPort();
        } else if (SolverLocal) {
            const TSockAddrLocal* const solver = dynamic_cast<const TSockAddrLocal*>(batch->Solver.Get());
            return solver && solver->ToString() == SolverLocal->ToString();
        } else {
            return false;
        }
    }
};

template <class T, class E>
void TClient<T, E>::TImpl::Run() {
    while (TImpl::RefCount() != 1) {
        TGuard<TMutex> init(Mutex);

        TInstant waitDeadline = Min(NCore::ReconnectTimeout, NCore::HeartbeatTimeout).ToDeadLine();

        if (!Deadlines.empty()) {
            waitDeadline = Min(waitDeadline, (*Deadlines.begin())->Deadline);
        }

        init.Release();

        size_t eventsGot = Poller.WaitD(PollerEvents.Data(), PollerEvents.Size(), waitDeadline);

        TGuard<TMutex> guard(Mutex);

        const TInstant now = TInstant::Now();

        for (TPoller::TEvent* i = PollerEvents.Data(); i != PollerEvents.Data() + eventsGot; ++i) try {
            TBatch* batch = reinterpret_cast<TBatch*>(TPoller::ExtractEvent(i));

            if (TPoller::ExtractStatus(i) != 0) {
                ythrow NCore::TDisconnection() << "connection error"sv;
            }

            if (TPoller::ExtractFilter(i) & CONT_POLL_READ) {
                batch->Recv();
            }
            if (TPoller::ExtractFilter(i) & CONT_POLL_WRITE && batch->Send()) {
                batch->LastHeartbeat = now;

                if (batch->LastReconnect != TInstant::Max()) {
                    for (typename TRequests::const_iterator it = Requests.begin(); it != Requests.end(); ++it) {
                        if (it->Batch == batch) {
                            BatchEvents.Set(TEvent(it->Userdata, CONNECTED, TString()));
                        }
                    }

                    ThreadEvent.Signal();
                    batch->LastReconnect = TInstant::Max();
                }
            }

            batch->UpdatePoller(Poller);
        } catch (const NCore::TBadVersion& e) {
            TBatchHandle batch(reinterpret_cast<TBatch*>(TPoller::ExtractEvent(i)));

            for (typename TRequests::iterator it = Requests.begin(); it != Requests.end();) {
                const typename TRequests::iterator request = it++;

                if (request->Batch == batch) {
                    if (request->State == TRequest::S_REQUESTED) {
                        Deadlines.EraseByPointer(&*request);
                    }

                    BatchEvents.Set(TEvent(request->Userdata, BADVERSION, e.what()));
                    RequestEvents.erase(request->Userdata);

                    Requests.erase(request);

                    ThreadEvent.Signal();
                }
            }
        } catch (const NCore::TDisconnection& e) {
            TBatchHandle batch(reinterpret_cast<TBatch*>(TPoller::ExtractEvent(i)));

            if (batch->LastReconnect == TInstant::Max() || e.Sent) {
                batch->ResetQueue();

                typedef THashMap<const TDefinition*, NCore::TKey> TDefinitions;
                TDefinitions definitions;

                for (typename TRequests::iterator it = Requests.begin(); it != Requests.end(); ++it)
                    if (it->Batch == batch) {
                        const std::pair<TDefinitions::iterator, bool> definition = definitions.insert(TDefinitions::value_type(it->Definition.Get(), it->Key));

                        if (definition.second) {
                            it->Batch->Enqueue(NCore::TDefineMessage(it->RejectActionId, it->Key, *it->Definition).Pack());
                        } else {
                            it->Batch->Enqueue(NCore::TDefineMessage(it->RejectActionId, it->Key, definition.first->second).Pack());
                        }

                        if (!it->Details.Empty()) {
                            it->Batch->Enqueue(NCore::TDetailsMessage(it->Key, it->Details).Pack());
                        }

                        if (it->State == TRequest::S_REQUESTED) {
                            it->Batch->Enqueue(NCore::TRequestMessage(it->GrantActionId, it->Key).Pack());
                        } else if (it->State == TRequest::S_GRANTED) {
                            it->Batch->Enqueue(NCore::TClaimMessage(0, it->Key).Pack());
                        }
                    }

                batch->LastHeartbeat = TInstant::Zero();
                batch->LastReconnect = now;
            }

            Poller.Remove(*batch->GetSocket());
        }

        for (typename TDeadlines::iterator i = Deadlines.begin(); i != Deadlines.end() && (*i)->Deadline <= now;) {
            const typename TDeadlines::iterator request = i++;

            (*request)->Batch->Enqueue(NCore::TDisclaimMessage((*request)->Key).Pack());

            (*request)->State = TRequest::S_DEFINED;

            RequestEvents.Set(TEvent((*request)->Userdata, EXPIRED, TString()));
            ThreadEvent.Signal();

            Deadlines.erase(request);

            (*request)->Batch->UpdatePoller(Poller);
        }

        for (typename TBatches::iterator i = Batches.begin(); i != Batches.end();) {
            const typename TBatches::iterator batch = i++;

            if ((*batch)->RefCount() == 1) {
                Poller.Remove(*(*batch)->GetSocket());
                Batches.erase(batch);
            } else if ((*batch)->LastReconnect != TInstant::Max() && (*batch)->LastReconnect + NCore::ReconnectTimeout <= now) {
                (*batch)->Connect();
                (*batch)->UpdatePoller(Poller);

                if ((*batch)->LastHeartbeat == TInstant::Zero()) {
                    for (typename TRequests::const_iterator it = Requests.begin(); it != Requests.end(); ++it) {
                        if (it->Batch == *batch) {
                            BatchEvents.Set(TEvent(it->Userdata, RECONNECTING, TString()));
                        }
                    }

                    ThreadEvent.Signal();
                }

                (*batch)->LastHeartbeat = now;
                (*batch)->LastReconnect = now;
            } else if (!(*batch)->Enqueued() && NCore::HeartbeatTimeout != TDuration::Max() && (*batch)->LastHeartbeat + NCore::HeartbeatTimeout <= now) {
                (*batch)->Enqueue(NCore::THeartbeatMessage().Pack());
                (*batch)->UpdatePoller(Poller);
            }
        }
    }
}

template <class T, class E>
void TClient<T, E>::TImpl::Define(typename TTypeTraits<T>::TFuncParam userdata, const ISockAddr& solver, const TDefinition& definition) {
    std::pair<typename TRequests::iterator, bool> request = Requests.InsertByUserdata(userdata);

    try {
        THolder<TDefinition> newDefinition(new TDefinition(definition));

        const NCore::TActionId RejectActionId = request.first->RejectActionId + 1;
        const NCore::TActionId GrantActionId = request.first->GrantActionId + 1;

        TAutoPtr<NCore::TPackedMessage> defineMessage(NCore::TDefineMessage(RejectActionId, request.first->Key, *newDefinition).Pack());
        TAutoPtr<NCore::TPackedMessage> requestMessage;
        TAutoPtr<NCore::TPackedMessage> claimMessage;

        if (!TSameSolverPredicate(solver)(request.first->Batch)) {
            typename TBatches::const_iterator batch = FindIf(Batches.begin(), Batches.end(), TSameSolverPredicate(solver));

            if (batch == Batches.end()) {
                batch = Batches.insert(new TBatch(this, solver)).first;
            }

            if (!request.second) {
                if (request.first->State == TRequest::S_REQUESTED) {
                    NCore::TRequestMessage(GrantActionId, request.first->Key).Pack().Swap(requestMessage);
                } else if (request.first->State == TRequest::S_GRANTED) {
                    NCore::TClaimMessage(0, request.first->Key).Pack().Swap(claimMessage);
                }

                request.first->Batch->EnqueueOrGroup(NCore::TUndefMessage(request.first->Key).Pack(), Grouping);
                request.first->Batch->UpdatePoller(Poller);
            }

            request.first->Batch = *batch;
        }

        request.first->Definition.Reset(newDefinition.Release());

        request.first->Batch->EnqueueOrGroup(defineMessage, Grouping);
        request.first->RejectActionId = RejectActionId;

        if (requestMessage.Get()) {
            request.first->Batch->EnqueueOrGroup(requestMessage, Grouping);
            request.first->GrantActionId = GrantActionId;
        } else if (claimMessage.Get()) {
            request.first->Batch->EnqueueOrGroup(claimMessage, Grouping);
            request.first->GrantActionId = GrantActionId;
        }

        if (request.first->Batch->LastReconnect == TInstant::Max()) {
            BatchEvents.Set(TEvent(userdata, CONNECTED, TString()));
            ThreadEvent.Signal();
        } else if (request.first->Batch->LastHeartbeat != TInstant::Zero()) {
            BatchEvents.Set(TEvent(userdata, RECONNECTING, TString()));
            ThreadEvent.Signal();
        }
    } catch (...) {
        if (request.second) {
            Requests.erase(request.first);
        }
        throw;
    }

    request.first->Batch->UpdatePoller(Poller);
}

template <class T, class E>
void TClient<T, E>::TImpl::Define(typename TTypeTraits<T>::TFuncParam userdata, typename TTypeTraits<T>::TFuncParam origin) {
    typename TRequests::const_iterator original = Requests.FindByUserdata(origin);

    if (original == Requests.end()) {
        ythrow TNotDefined() << "the original request is not defined"sv;
    }

    std::pair<typename TRequests::iterator, bool> request = Requests.InsertByUserdata(userdata);

    try {
        const NCore::TActionId RejectActionId = request.first->RejectActionId + 1;
        const NCore::TActionId GrantActionId = request.first->GrantActionId + 1;

        TAutoPtr<NCore::TPackedMessage> defineMessage(NCore::TDefineMessage(RejectActionId, request.first->Key, original->Key).Pack());
        TAutoPtr<NCore::TPackedMessage> requestMessage;
        TAutoPtr<NCore::TPackedMessage> claimMessage;

        if (request.first->Batch != original->Batch) {
            if (!request.second) {
                if (request.first->State == TRequest::S_REQUESTED) {
                    NCore::TRequestMessage(GrantActionId, request.first->Key).Pack().Swap(requestMessage);
                } else if (request.first->State == TRequest::S_GRANTED) {
                    NCore::TClaimMessage(0, request.first->Key).Pack().Swap(claimMessage);
                }

                request.first->Batch->EnqueueOrGroup(NCore::TUndefMessage(request.first->Key).Pack(), Grouping);
                request.first->Batch->UpdatePoller(Poller);
            }

            request.first->Batch = original->Batch;
        }

        request.first->Definition = original->Definition;

        request.first->Batch->EnqueueOrGroup(defineMessage, Grouping);
        request.first->RejectActionId = RejectActionId;

        if (requestMessage.Get()) {
            request.first->Batch->EnqueueOrGroup(requestMessage, Grouping);
            request.first->GrantActionId = GrantActionId;
        } else if (claimMessage.Get()) {
            request.first->Batch->EnqueueOrGroup(claimMessage, Grouping);
            request.first->GrantActionId = GrantActionId;
        }

        if (request.first->Batch->LastReconnect == TInstant::Max()) {
            BatchEvents.Set(TEvent(userdata, CONNECTED, TString()));
            ThreadEvent.Signal();
        } else if (request.first->Batch->LastHeartbeat != TInstant::Zero()) {
            BatchEvents.Set(TEvent(userdata, RECONNECTING, TString()));
            ThreadEvent.Signal();
        }
    } catch (...) {
        if (request.second) {
            Requests.erase(request.first);
        }
        throw;
    }

    request.first->Batch->UpdatePoller(Poller);
}

template <class T, class E>
void TClient<T, E>::TImpl::Undef(typename TTypeTraits<T>::TFuncParam userdata) {
    typename TRequests::iterator request = Requests.FindByUserdata(userdata);

    if (request != Requests.end()) {
        request->Batch->EnqueueOrGroup(NCore::TUndefMessage(request->Key).Pack(), Grouping);

        if (request->State == TRequest::S_REQUESTED) {
            Deadlines.EraseByPointer(&*request);
        }

        BatchEvents.erase(request->Userdata);
        RequestEvents.erase(request->Userdata);

        request->Batch->UpdatePoller(Poller);

        Requests.erase(request);
    }
}

template <class T, class E>
void TClient<T, E>::TImpl::Details(typename TTypeTraits<T>::TFuncParam userdata, const TDetails& details) {
    typename TRequests::iterator request = Requests.FindByUserdata(userdata);

    if (request == Requests.end()) {
        ythrow TNotDefined() << "the request is not defined"sv;
    }

    if (details.Empty()) {
        return;
    }

    TAutoPtr<NCore::TPackedMessage> message(NCore::TDetailsMessage(request->Key, details).Pack());

    request->Details.MergeFrom(details);

    request->Batch->EnqueueOrGroup(message, Grouping);
    request->Batch->UpdatePoller(Poller);
}

template <class T, class E>
void TClient<T, E>::TImpl::Request(typename TTypeTraits<T>::TFuncParam userdata, TInstant deadline) {
    typename TRequests::iterator request = Requests.FindByUserdata(userdata);

    if (request == Requests.end()) {
        ythrow TNotDefined() << "the request is not defined"sv;
    }

    const NCore::TActionId GrantActionId = request->GrantActionId + 1;

    request->Batch->EnqueueOrGroup(NCore::TRequestMessage(GrantActionId, request->Key).Pack(), Grouping);
    request->GrantActionId = GrantActionId;

    if (request->State == TRequest::S_REQUESTED) {
        Deadlines.EraseByPointer(&*request);
    }

    request->State = TRequest::S_REQUESTED;
    request->Deadline = deadline;

    Deadlines.insert(&*request);

    RequestEvents.erase(request->Userdata);

    request->Batch->UpdatePoller(Poller);
}

template <class T, class E>
void TClient<T, E>::TImpl::Claim(typename TTypeTraits<T>::TFuncParam userdata, bool asynchronous) {
    typename TRequests::iterator request = Requests.FindByUserdata(userdata);

    if (request == Requests.end()) {
        ythrow TNotDefined() << "the request is not defined"sv;
    }

    const NCore::TActionId GrantActionId = request->GrantActionId + 1;

    request->Batch->EnqueueOrGroup(NCore::TClaimMessage(asynchronous ? 0 : GrantActionId, request->Key).Pack(), Grouping);
    request->GrantActionId = GrantActionId;

    if (asynchronous) {
        RequestEvents.Set(TEvent(userdata, GRANTED, TString()));
        ThreadEvent.Signal();
    } else {
        RequestEvents.erase(request->Userdata);
    }

    if (request->State == TRequest::S_REQUESTED) {
        Deadlines.EraseByPointer(&*request);
    }

    request->State = TRequest::S_GRANTED;

    request->Batch->UpdatePoller(Poller);
}

template <class T, class E>
void TClient<T, E>::TImpl::Disclaim(typename TTypeTraits<T>::TFuncParam userdata) {
    typename TRequests::iterator request = Requests.FindByUserdata(userdata);

    if (request == Requests.end()) {
        ythrow TNotDefined() << "the request is not defined"sv;
    }

    const NCore::TActionId GrantActionId = request->GrantActionId + 1;

    request->Batch->EnqueueOrGroup(NCore::TDisclaimMessage(request->Key).Pack(), Grouping);
    request->GrantActionId = GrantActionId;

    if (request->State == TRequest::S_REQUESTED) {
        Deadlines.EraseByPointer(&*request);
    }

    request->State = TRequest::S_DEFINED;

    RequestEvents.erase(request->Userdata);

    request->Batch->UpdatePoller(Poller);
}

template <class T, class E>
inline void TClient<T, E>::TImpl::StartGrouping() {
    Y_ASSERT(!Grouping.Get());
    Grouping.Get() = true;
}

template <class T, class E>
inline void TClient<T, E>::TImpl::FinishGrouping() {
    Y_ASSERT(Grouping.Get());
    Grouping.Get() = false;
    for (typename TBatches::iterator batch = Batches.begin(); batch != Batches.end(); ++batch) {
        (*batch)->Enqueue(NCore::TGroupMessage((*batch)->Group.Get().Size()).Pack());
        (*batch)->Enqueue((*batch)->Group.Get());
        (*batch)->UpdatePoller(Poller);
    }
}

template <class T, class E>
void TClient<T, E>::TImpl::PullEvents(TEvents& events) {
    events.insert(events.end(), BatchEvents.begin(), BatchEvents.end());
    BatchEvents.clear();
    events.insert(events.end(), RequestEvents.begin(), RequestEvents.end());
    RequestEvents.clear();
    ThreadEvent.Reset();
}

template <class T, class E>
inline void TClient<T, E>::TImpl::PullEvents(TEvents& events, typename TTypeTraits<T>::TFuncParam userdata) {
    const typename TEventsMap::iterator batchEvent = BatchEvents.find(userdata);
    if (batchEvent != BatchEvents.end()) {
        events.push_back(*batchEvent);
        BatchEvents.erase(batchEvent);
    }
    const typename TEventsMap::iterator requestEvent = RequestEvents.find(userdata);
    if (requestEvent != RequestEvents.end()) {
        events.push_back(*requestEvent);
        RequestEvents.erase(requestEvent);
    }
    if (BatchEvents.empty() && RequestEvents.empty()) {
        ThreadEvent.Reset();
    }
}

template <class T, class E>
inline void TClient<T, E>::TImpl::SetCustomEvent(const TEvent& event) {
    if (Requests.FindByUserdata(event.Userdata) != Requests.end())
        ythrow TDefined() << "unable to set custom event for defined request"sv;

    switch (event.Status) {
    case BADVERSION:
    case CONNECTED:
    case RECONNECTING:
        BatchEvents.Set(event);
        break;
    case GRANTED:
    case EXPIRED:
    case REJECTED:
        RequestEvents.Set(event);
        break;
    default:
        Y_FAIL("Unknown event status specified");
    }
}

template <class T, class E>
inline TAutoPtr< TVector<SOCKET> > TClient<T, E>::TImpl::GetSockets() const {
    TAutoPtr< TVector<SOCKET> > sockets(new TVector<SOCKET>());

    sockets->reserve(Batches.size());

    for (typename TBatches::const_iterator batch = Batches.begin(); batch != Batches.end(); ++batch) {
        if ((*batch)->GetSocket()) {
            sockets->push_back(*(*batch)->GetSocket());
        }
    }

    return sockets;
}

// TClient methods implementation

template <class T, class E>
TClient<T, E>::TClient()
    : TThread(&TImpl::ClientThreadProc, this)
    , Impl(new TImpl)
{
}

template <class T, class E>
TClient<T, E>::~TClient() {
    Detach();
}

template <class T, class E>
void TClient<T, E>::Start() {
    TThread::Start();
}

template <class T, class E>
inline void TClient<T, E>::Reset() {
    TIntrusivePtr<TImpl> impl(new TImpl);
    DoSwap(Impl, impl);
    impl->ThreadEvent.Signal();
    TThread::Detach();
}

template <class T, class E>
void TClient<T, E>::Define(typename TTypeTraits<T>::TFuncParam userdata, const ISockAddr& solver, const TDefinition& definition) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Define(userdata, solver, definition);
}

template <class T, class E>
void TClient<T, E>::Define(typename TTypeTraits<T>::TFuncParam userdata, typename TTypeTraits<T>::TFuncParam origin) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Define(userdata, origin);
}

template <class T, class E>
void TClient<T, E>::Undef(typename TTypeTraits<T>::TFuncParam userdata) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Undef(userdata);
}

template <class T, class E>
inline void TClient<T, E>::Details(typename TTypeTraits<T>::TFuncParam userdata, const TDetails& details) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Details(userdata, details);
}

template <class T, class E>
void TClient<T, E>::Request(typename TTypeTraits<T>::TFuncParam userdata, TInstant deadline) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Request(userdata, deadline);
}

template <class T, class E>
void TClient<T, E>::Claim(typename TTypeTraits<T>::TFuncParam userdata, bool asynchronous) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Claim(userdata, asynchronous);
}

template <class T, class E>
void TClient<T, E>::Disclaim(typename TTypeTraits<T>::TFuncParam userdata) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->Disclaim(userdata);
}

template <class T, class E>
inline void TClient<T, E>::StartGrouping() {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->StartGrouping();
}

template <class T, class E>
inline void TClient<T, E>::FinishGrouping() {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->FinishGrouping();
}

template <class T, class E>
void TClient<T, E>::PullEvents(TEvents& events) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->PullEvents(events);
}

template <class T, class E>
void TClient<T, E>::PullEvents(TEvents& events, typename TTypeTraits<T>::TFuncParam userdata) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->PullEvents(events, userdata);
}

template <class T, class E>
inline bool TClient<T, E>::WaitD(TInstant deadline) {
    TIntrusivePtr<TImpl> impl(Impl);
    return impl->ThreadEvent.WaitD(deadline);
}

template <class T, class E>
inline void TClient<T, E>::SetCustomEvent(const TEvent& event) {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    impl->SetCustomEvent(event);
}

template <class T, class E>
inline void TClient<T, E>::Signal() {
    TIntrusivePtr<TImpl> impl(Impl);
    impl->ThreadEvent.Signal();
}

template <class T, class E>
inline TAutoPtr< TVector<SOCKET> > TClient<T, E>::GetSockets() const {
    TIntrusivePtr<TImpl> impl(Impl);
    TGuard<TMutex> guard(impl->Mutex);
    return impl->GetSockets();
}

}
