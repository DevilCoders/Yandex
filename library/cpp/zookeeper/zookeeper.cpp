#include "zookeeper.h"

#include <contrib/libs/zookeeper/include/zookeeper.h>

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>
#include <util/system/error.h>

#include <utility>

using namespace NThreading;

namespace NZooKeeper {
    namespace {
        void SetServersResolutionDelay(zhandle_t* zHandle, const TDuration& delay) {
            if (delay == TDuration::Max()) {
                zoo_set_servers_resolution_delay(zHandle, -1);
            } else {
                zoo_set_servers_resolution_delay(zHandle, delay.MilliSeconds());
            }
        }

        template <typename T>
        class TCallbackState {
            TPromise<T> Promise = NewPromise<T>();

        public:
            bool CheckResponse(int rc) {
                if (rc != ZOK) {
                    Promise.SetException(zerror(rc));
                    return false;
                }

                return true;
            }

            void SetValue(T&& value) {
                Promise.SetValue(value);
            }

            TFuture<T> GetFuture() const {
                return Promise.GetFuture();
            }
        };

        template <>
        class TCallbackState<void> {
            TPromise<void> Promise = NewPromise<void>();

        public:
            bool CheckResponse(int rc) {
                if (rc != ZOK) {
                    Promise.SetException(zerror(rc));
                    return false;
                }

                return true;
            }

            void SetValue() {
                Promise.SetValue();
            }

            TFuture<void> GetFuture() const {
                return Promise.GetFuture();
            }
        };

        void InitCreateOperation(const TZooOperation& op, zoo_op_t* zooOp) {
            const auto& createOp = static_cast<const TZooCreateOperation&>(op);
            zoo_create_op_init(
                zooOp,
                createOp.Path.data(),
                createOp.Data.data(),
                createOp.Data.size(),
                &createOp.ACLVector,
                createOp.CreateMode,
                NULL,
                0);
        }

        void InitDeleteOperation(const TZooOperation& op, zoo_op_t* zooOp) {
            const auto& deleteOp = static_cast<const TZooDeleteOperation&>(op);
            zoo_delete_op_init(zooOp, deleteOp.Path.data(), deleteOp.Version);
        }

        void InitSetOperation(const TZooOperation& op, zoo_op_t* zooOp) {
            const auto& setOp = static_cast<const TZooSetOperation&>(op);
            zoo_set_op_init(
                zooOp,
                setOp.Path.data(),
                setOp.Data.data(),
                setOp.Data.size(),
                setOp.Version,
                setOp.Stat.Get());
        }

        void InitCheckOperation(const TZooOperation& op, zoo_op_t* zooOp) {
            const auto& checkOp = static_cast<const TZooCheckOperation&>(op);
            zoo_check_op_init(zooOp, checkOp.Path.data(), checkOp.Version);
        }

        void InitTransactionOperations(const TZooOperations& ops, TVector<zoo_op_t>& zooOps) {
            const auto n = ops.size();
            for (size_t i = 0; i < n; ++i) {
                auto* zooOp = &zooOps[i];
                const auto& op = *ops[i];
                switch (op.Type) {
                    case TZooOperation::CreateOp:
                        InitCreateOperation(op, zooOp);
                        break;
                    case TZooOperation::DeleteOp:
                        InitDeleteOperation(op, zooOp);
                        break;
                    case TZooOperation::SetOp:
                        InitSetOperation(op, zooOp);
                        break;
                    case TZooOperation::CheckOp:
                        InitCheckOperation(op, zooOp);
                        break;
                }
            }
        }

    }

    ////////////////////////////////////////////////////////////////////////////////

    TError::TError(int error)
        : ZooErrorCode(error)
    {
        *this << zerror(error);
    }

    TNodeExistsError::TNodeExistsError(int error)
        : TError(error)
    {
    }

    TNodeNotExistsError::TNodeNotExistsError(int error)
        : TError(error)
    {
    }

    TConnectionLostError::TConnectionLostError(int error)
        : TError(error)
    {
    }

    TInvalidStateError::TInvalidStateError(int error)
        : TError(error)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////

    class TSessionId::TImpl {
    private:
        clientid_t ClientId;

    public:
        TImpl(const clientid_t& clientId)
            : ClientId(clientId)
        {
        }

        const clientid_t& GetClientId() const noexcept {
            return ClientId;
        }
    };

    TSessionId::TSessionId(TImpl* impl)
        : Impl(impl)
    {
    }

    TSessionId::~TSessionId() {
    }

    ////////////////////////////////////////////////////////////////////////////////

    class TZooKeeper::TImpl {
    private:
        IWatcher* Watcher;

        zhandle_t* ZHandle;

        static void WatcherFunc(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx);
        static void GetCallback(int rc, const char* value, int value_len,
                                const struct Stat* stat, const void* data);

        static void ProcessException(int error);

    public:
        TImpl(const TZooKeeperOptions& options);
        TImpl(const TString& connectString, int sessionTimeout, IWatcher* watcher, TSessionIdPtr sessionId,
              bool canBeReadOnly, const TDuration& serversResolutionDelay);

        ~TImpl();

        TSessionIdPtr GetSessionId() const noexcept;

        void Register(IWatcher* watcher) noexcept;
        void Close();

        TString Create(const TString& path, const TString& data, const TVector<TACL>& acl, ECreateMode createMode);
        void CreateHierarchy(const TFsPath& path, const TVector<TACL>& acl);
        void Delete(const TString& path, int version);

        TStatPtr Exists(const TString& path, bool watch);

        TVector<TString> GetChildren(const TString& path, bool watch);
        TString GetData(const TString& path, bool watch, TStat* stat);
        void GetData(const TString& path,
                     bool watch,
                     const TDataCallback& cb);
        void SetData(const TString& path, const TString& data, int version);
        bool ForceSetData(const TString& path, const TString& data);

        void DoTransaction(const TZooOperations& operations);

        TFuture<TString> CreateAsync(const TString& path, const TString& data, const TVector<TACL>& acl, ECreateMode createMode);
        TFuture<void> DeleteAsync(const TString& path, int version);

        TFuture<TStatPtr> ExistsAsync(const TString& path, bool watch);
        TFuture<TVector<TString>> GetChildrenAsync(const TString& path, bool watch);

        TFuture<TString> GetDataAsync(const TString& path, bool watch = false);
        TFuture<std::pair<TString, TStatPtr>> GetDataWithStatAsync(const TString& path, bool watch = false);

        TFuture<void> SetDataAsync(const TString& path, const TString& data, int version = -1);

        TFuture<void> DoTransactionAsync(const TZooOperations& operations);
    };

    void TZooKeeper::TImpl::WatcherFunc(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
        Y_UNUSED(zh);

        IWatcher* watcher = static_cast<IWatcher*>(watcherCtx);
        TWatchedEvent event(static_cast<EEventType>(type), static_cast<EKeeperState>(state), path);
        if (watcher) {
            watcher->Process(event);
        }
    }

    TZooKeeper::TImpl::TImpl(const TZooKeeperOptions& options)
        : Watcher(options.Watcher)
    {
        zoo_set_debug_level((ZooLogLevel)options.LogLevel);

        ZHandle = zookeeper_init(options.Address.data(), WatcherFunc, options.Timeout.MilliSeconds(),
                                 !!options.SessionId ? &(options.SessionId->Impl->GetClientId()) : nullptr, Watcher, 0);
        if (!ZHandle)
            throw TSystemError();

        if (options.LogFunction != nullptr) {
            zoo_set_log_callback(ZHandle, options.LogFunction);
        }

        SetServersResolutionDelay(ZHandle, options.ServersResolutionDelay);
    }

    TZooKeeper::TImpl::TImpl(const TString& connectString, int sessionTimeout, IWatcher* watcher,
                             TSessionIdPtr sessionId, bool canBeReadOnly, const TDuration& serversResolutionDelay)
        : Watcher(watcher)
    {
        Y_UNUSED(canBeReadOnly);

        ZHandle = zookeeper_init(connectString.data(), WatcherFunc, sessionTimeout,
                                 sessionId.Get() ? &sessionId->Impl->GetClientId() : nullptr, Watcher, 0);
        if (!ZHandle)
            throw TSystemError();

        SetServersResolutionDelay(ZHandle, serversResolutionDelay);
    }

    TZooKeeper::TImpl::~TImpl() {
        Close();
    }

    TSessionIdPtr TZooKeeper::TImpl::GetSessionId() const noexcept {
        const clientid_t* client_id = zoo_client_id(ZHandle);
        return new TSessionId(new TSessionId::TImpl(*client_id));
    }

    void TZooKeeper::TImpl::Register(IWatcher* watcher) noexcept {
        Watcher = watcher;
        zoo_set_context(ZHandle, Watcher);
    }

    void TZooKeeper::TImpl::Close() {
        if (ZHandle == nullptr) {
            return;
        }
        int error = zookeeper_close(ZHandle);
        ZHandle = nullptr;
        ProcessException(error);
    }

    void TZooKeeper::TImpl::CreateHierarchy(const TFsPath& path, const TVector<TACL>& acl) {
        if (path.GetPath() == "/") {
            return;
        }

        bool exists = false;
        try {
            exists = !!Exists(path.GetPath(), false);
        } catch (...) {
            exists = false;
        }

        if (!exists) {
            CreateHierarchy(path.Parent(), acl);
            try {
                Create(path.Parent() / path.GetName(), "", acl, CM_PERSISTENT);
            } catch (TNodeExistsError&) {
            }
        }
    }

    TString TZooKeeper::TImpl::Create(const TString& path, const TString& data, const TVector<TACL>& acl,
                                      ECreateMode createMode) {
        struct ACL_vector aclVector;
        aclVector.count = acl.size();
        aclVector.data = const_cast<TACL*>(&acl.front());

        char pathCreated[1024];

        int error = zoo_create(ZHandle, path.data(), data.data(), data.size(), &aclVector, createMode, pathCreated,
                               sizeof(pathCreated));
        ProcessException(error);

        return pathCreated;
    }

    void TZooKeeper::TImpl::Delete(const TString& path, int version) {
        int error = zoo_delete(ZHandle, path.data(), version);
        ProcessException(error);
    }

    TStatPtr TZooKeeper::TImpl::Exists(const TString& path, bool watch) {
        TStatPtr stat = new TStat;
        int error = zoo_exists(ZHandle, path.data(), watch, stat.Get());
        if (error != ZOK) {
            if (error == ZNONODE)
                stat.Reset(nullptr);
            else
                ProcessException(error);
        }

        return stat;
    }

    TVector<TString> TZooKeeper::TImpl::GetChildren(const TString& path, bool watch) {
        struct String_vector strings;
        int error = zoo_get_children(ZHandle, path.data(), watch, &strings);
        ProcessException(error);

        TVector<TString> result;
        for (int i = 0; i < strings.count; ++i) {
            result.push_back(strings.data[i]);
        }
        deallocate_String_vector(&strings);

        return result;
    }

    TString TZooKeeper::TImpl::GetData(const TString& path, bool watch, TStat* stat) {
        int bufSize = 64 * 1024;
        TBuffer buffer(bufSize);
        int len = buffer.Capacity();

        TStat stat2;
        int error = zoo_get(ZHandle, path.data(), watch, buffer.Data(), &len, &stat2);
        ProcessException(error);

        if (stat2.dataLength > bufSize) {
            buffer.Resize(stat2.dataLength);
            len = stat2.dataLength;

            error = zoo_get(ZHandle, path.data(), watch, buffer.Data(), &len, &stat2);
            ProcessException(error);
        }

        if (stat) {
            *stat = stat2;
        }

        return TString(buffer.Data(), len < 0 ? 0 : len);
    }

    void TZooKeeper::TImpl::SetData(const TString& path,
                                    const TString& data,
                                    int version) {
        int error = zoo_set(ZHandle, path.data(), data.data(), data.size(), version);
        ProcessException(error);
    }

    void TZooKeeper::TImpl::DoTransaction(const TZooOperations& operations) {
        if (!operations) {
            return;
        }

        const auto n = operations.size();
        TVector<zoo_op_t> zooOperations(n);
        InitTransactionOperations(operations, zooOperations);

        // not initialized because not checked
        TVector<zoo_op_result_t> results(n);

        int error = zoo_multi(ZHandle, zooOperations.size(), &zooOperations[0], &results[0]);
        ProcessException(error);
    }

    void TZooKeeper::TImpl::GetData(const TString& path,
                                    bool watch,
                                    const TDataCallback& cb) {
        zoo_aget(ZHandle, path.data(), watch, TZooKeeper::TImpl::GetCallback,
                 new ::std::pair<TString, TDataCallback>(path, cb));
    }

    void TZooKeeper::TImpl::GetCallback(int rc, const char* value, int value_len,
                                        const struct Stat* stat, const void* data) {
        TDataCallbackResult result;
        THolder<std::pair<TString, TDataCallback>> callback((std::pair<TString, TDataCallback>*)data);

        result.ProcessException = std::bind(&TZooKeeper::TImpl::ProcessException, rc);
        if (stat) {
            result.Stat = *(TStat*)(stat);
        }
        result.Path = callback->first;

        // value_len can be -1 in case the node has no value at all
        if (value_len > 0) {
            result.Data = TString(value, value_len);
        }

        (callback->second)(result);
    }

    TFuture<TString> TZooKeeper::TImpl::CreateAsync(const TString& path, const TString& data, const TVector<TACL>& acl,
                                                    ECreateMode createMode) {
        using TCallbackType = TCallbackState<TString>;
        auto state = MakeHolder<TCallbackType>();
        auto cb = [](int rc, const char* value, const void* theData) {
            THolder<TCallbackType> theState((TCallbackType*)theData);
            if (theState->CheckResponse(rc)) {
                theState->SetValue(value);
            }
        };

        ACL_vector aclVector;
        aclVector.count = acl.size();
        aclVector.data = const_cast<TACL*>(acl.data());

        auto future = state->GetFuture();
        auto rc = zoo_acreate(ZHandle, path.data(), data.data(), data.size(), &aclVector, createMode, cb, state.Get());
        ProcessException(rc);

        Y_UNUSED(state.Release());
        return future;
    }

    TFuture<void> TZooKeeper::TImpl::DeleteAsync(const TString& path, int version) {
        using TCallbackType = TCallbackState<void>;
        auto state = MakeHolder<TCallbackType>();
        auto cb = [](int rc, const void* data) {
            THolder<TCallbackType> theState((TCallbackType*)data);
            if (theState->CheckResponse(rc)) {
                theState->SetValue();
            }
        };

        auto future = state->GetFuture();
        auto rc = zoo_adelete(ZHandle, path.data(), version, cb, state.Get());
        ProcessException(rc);

        Y_UNUSED(state.Release());
        return future;
    }

    TFuture<TStatPtr> TZooKeeper::TImpl::ExistsAsync(const TString& path, bool watch) {
        using TCallbackType = TCallbackState<TStatPtr>;
        auto state = MakeHolder<TCallbackType>();
        auto cb = [](int rc, const Stat* stat, const void* data) {
            THolder<TCallbackType> theState((TCallbackType*)data);
            if (rc == ZNONODE) {
                theState->SetValue(nullptr);
            } else if (theState->CheckResponse(rc)) {
                theState->SetValue(new TStat(*stat));
            };
        };

        auto future = state->GetFuture();
        int rc = zoo_aexists(ZHandle, path.data(), watch, cb, state.Get());
        ProcessException(rc);

        Y_UNUSED(state.Release());
        return future;
    }

    TFuture<TVector<TString>> TZooKeeper::TImpl::GetChildrenAsync(const TString& path, bool watch) {
        using TCallbackType = TCallbackState<TVector<TString>>;
        auto state = MakeHolder<TCallbackType>();
        auto cb = [](int rc, const String_vector* strings, const void* data) {
            THolder<TCallbackType> theState((TCallbackType*)data);
            if (theState->CheckResponse(rc)) {
                TVector<TString> value(strings->count);
                for (int i = 0; i < strings->count; ++i) {
                    value[i] = strings->data[i];
                }
                theState->SetValue(std::move(value));
            }
        };

        auto future = state->GetFuture();
        int error = zoo_aget_children(ZHandle, path.data(), watch, cb, state.Get());
        ProcessException(error);

        Y_UNUSED(state.Release());
        return future;
    }

    TFuture<TString> TZooKeeper::TImpl::GetDataAsync(const TString& path, bool watch) {
        return GetDataWithStatAsync(path, watch).Apply([](const TFuture<std::pair<TString, TStatPtr>>& res) {
            return res.GetValue().first;
        });
    }

    TFuture<std::pair<TString, TStatPtr>> TZooKeeper::TImpl::GetDataWithStatAsync(const TString& path, bool watch) {
        using TCallbackType = TCallbackState<std::pair<TString, TStatPtr>>;
        auto state = MakeHolder<TCallbackType>();
        auto cb = [](int rc, const char* value, int value_len, const Stat* stat, const void* data1) {
            THolder<TCallbackType> theState((TCallbackType*)data1);
            if (theState->CheckResponse(rc)) {
                TString data2;
                if (value_len > 0) {
                    data2 = TString(value, value_len);
                }

                TStatPtr stPtr;
                if (stat) {
                    stPtr = new TStat(*stat);
                }

                theState->SetValue(std::make_pair(std::move(data2), stPtr));
            }
        };

        auto future = state->GetFuture();
        int error = zoo_aget(ZHandle, path.data(), watch, cb, state.Get());
        ProcessException(error);

        Y_UNUSED(state.Release());
        return future;
    }

    TFuture<void> TZooKeeper::TImpl::SetDataAsync(const TString& path, const TString& data, int version) {
        using TCallbackType = TCallbackState<void>;
        auto state1 = MakeHolder<TCallbackType>();
        auto cb = [](int rc, const Stat*, const void* data2) {
            THolder<TCallbackType> state2((TCallbackType*)data2);
            if (state2->CheckResponse(rc)) {
                state2->SetValue();
            }
        };

        auto future = state1->GetFuture();
        auto rc = zoo_aset(ZHandle, path.data(), data.data(), data.size(), version, cb, state1.Get());
        ProcessException(rc);

        Y_UNUSED(state1.Release());
        return future;
    }

    TFuture<void> TZooKeeper::TImpl::DoTransactionAsync(const TZooOperations& operations) {
        if (!operations) {
            return MakeFuture();
        }

        struct TContext {
            TCallbackState<void> State;
            TVector<zoo_op_t> ZooOperations;
            TVector<zoo_op_result_t> Results; // not initialized because not checked

            TContext(size_t n)
                : ZooOperations(n)
                , Results(n)
            {
            }
        };

        const auto n = operations.size();
        auto context1 = MakeHolder<TContext>(n);
        InitTransactionOperations(operations, context1->ZooOperations);

        auto cb = [](int rc, const void* data) {
            THolder<TContext> context2((TContext*)data);
            if (context2->State.CheckResponse(rc)) {
                context2->State.SetValue();
            }
        };

        auto future = context1->State.GetFuture();
        auto rc = zoo_amulti(
            ZHandle,
            context1->ZooOperations.size(),
            &context1->ZooOperations[0],
            &context1->Results[0],
            cb,
            context1.Get());
        ProcessException(rc);

        Y_UNUSED(context1.Release());
        return future;
    }

    void TZooKeeper::TImpl::ProcessException(int error) {
        if (error != ZOK) {
            switch (error) {
                case ZNODEEXISTS:
                    throw TNodeExistsError(error);
                case ZNONODE:
                    throw TNodeNotExistsError(error);
                case ZCONNECTIONLOSS:
                    throw TConnectionLostError(error);
                case ZINVALIDSTATE:
                    throw TInvalidStateError(error);
                default:
                    throw TError(error);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////

    TZooKeeper::TZooKeeper(const TZooKeeperOptions& options)
        : Impl(new TImpl(options))
    {
    }

    TZooKeeper::TZooKeeper(const TString& connectString, int sessionTimeout, IWatcher* watcher,
                           bool canBeReadOnly, const TDuration& serversResolutionDelay)
        : Impl(new TImpl(connectString, sessionTimeout, watcher, nullptr, canBeReadOnly, serversResolutionDelay))
    {
    }

    TZooKeeper::TZooKeeper(const TString& connectString, int sessionTimeout, IWatcher* watcher,
                           TSessionIdPtr sessionId, bool canBeReadOnly, const TDuration& serversResolutionDelay)
        : Impl(new TImpl(connectString, sessionTimeout, watcher, sessionId, canBeReadOnly, serversResolutionDelay))
    {
    }

    TZooKeeper::~TZooKeeper() {
    }

    TSessionIdPtr TZooKeeper::GetSessionId() const noexcept {
        return Impl->GetSessionId();
    }

    void TZooKeeper::Register(IWatcher* watcher) noexcept {
        Impl->Register(watcher);
    }

    void TZooKeeper::Close() {
        Impl->Close();
    }

    TString TZooKeeper::Create(const TString& path, const TString& data, const TVector<TACL>& acl,
                               ECreateMode createMode) {
        return Impl->Create(path, data, acl, createMode);
    }

    void TZooKeeper::CreateHierarchy(const TFsPath& path, const TVector<TACL>& acl) {
        return Impl->CreateHierarchy(path, acl);
    }

    void TZooKeeper::Delete(const TString& path, int version) {
        Impl->Delete(path, version);
    }

    TStatPtr TZooKeeper::Exists(const TString& path, bool watch) {
        return Impl->Exists(path, watch);
    }

    TVector<TString> TZooKeeper::GetChildren(const TString& path, bool watch) {
        return Impl->GetChildren(path, watch);
    }

    TString TZooKeeper::GetData(const TString& path, bool watch, TStat* stat) {
        return Impl->GetData(path, watch, stat);
    }

    void TZooKeeper::SetData(const TString& path, const TString& data, int version) {
        return Impl->SetData(path, data, version);
    }

    void TZooKeeper::GetData(const TString& path,
                             bool watch,
                             const TDataCallback& cb) {
        Impl->GetData(path, watch, cb);
    }

    void TZooKeeper::DoTransaction(const TZooOperations& operations) {
        Impl->DoTransaction(operations);
    }

    TFuture<TString> TZooKeeper::CreateAsync(const TString& path, const TString& data, const TVector<TACL>& acl,
                                             ECreateMode createMode) {
        return Impl->CreateAsync(path, data, acl, createMode);
    }

    TFuture<void> TZooKeeper::DeleteAsync(const TString& path, int version) {
        return Impl->DeleteAsync(path, version);
    }

    TFuture<TStatPtr> TZooKeeper::ExistsAsync(const TString& path, bool watch) {
        return Impl->ExistsAsync(path, watch);
    }

    TFuture<TVector<TString>> TZooKeeper::GetChildrenAsync(const TString& path, bool watch) {
        return Impl->GetChildrenAsync(path, watch);
    }

    TFuture<TString> TZooKeeper::GetDataAsync(const TString& path, bool watch) {
        return Impl->GetDataAsync(path, watch);
    }

    TFuture<std::pair<TString, TStatPtr>> TZooKeeper::GetDataWithStatAsync(const TString& path, bool watch) {
        return Impl->GetDataWithStatAsync(path, watch);
    }

    TFuture<void> TZooKeeper::SetDataAsync(const TString& path, const TString& data, int version) {
        return Impl->SetDataAsync(path, data, version);
    }

    TFuture<void> TZooKeeper::DoTransactionAsync(const TZooOperations& operations) {
        return Impl->DoTransactionAsync(operations);
    }

    void TZooKeeper::Reset(const TZooKeeperOptions& options) {
        Impl.Reset(new TImpl(options));
    }

    void TZooKeeper::Reset(const TString& connectString, int sessionTimeout, IWatcher* watcher,
                           bool canBeReadOnly, const TDuration& serversResolutionDelay) {
        Impl.Reset(new TImpl(connectString, sessionTimeout, watcher, nullptr, canBeReadOnly, serversResolutionDelay));
    }

    void TZooKeeper::Reset(const TString& connectString, int sessionTimeout, IWatcher* watcher,
                           TSessionIdPtr sessionId, bool canBeReadOnly, const TDuration& serversResolutionDelay) {
        Impl.Reset(new TImpl(connectString, sessionTimeout, watcher, sessionId, canBeReadOnly, serversResolutionDelay));
    }

}
