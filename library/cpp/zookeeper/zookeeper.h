#pragma once

#include "defines.h"

#include <library/cpp/threading/future/future.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/datetime/base.h>
#include <util/folder/path.h>

#include <functional>

namespace NZooKeeper {
    struct TError: public yexception {
    private:
        int ZooErrorCode = 0;

    public:
        TError(int error);
        int GetZooErrorCode() const {
            return ZooErrorCode;
        }
    };

    struct TNodeExistsError: public TError {
        TNodeExistsError(int error);
    };

    struct TNodeNotExistsError: public TError {
        TNodeNotExistsError(int error);
    };

    struct TConnectionLostError: public TError {
        TConnectionLostError(int error);
    };

    struct TInvalidStateError: public TError {
        TInvalidStateError(int error);
    };

    struct TWatchedEvent {
        EEventType EventType;
        EKeeperState KeeperState;
        TString Path;

        TWatchedEvent(EEventType eventType, EKeeperState keeperState, const TString& path)
            : EventType(eventType)
            , KeeperState(keeperState)
            , Path(path)
        {
        }

        TString ToString() const {
            return "WatchedEvent EventType:" + ::ToString(EventType) + " KeeperState:" + ::ToString(KeeperState) + " Path:" + Path;
        }
    };

    class IWatcher {
    public:
        virtual ~IWatcher() {
        }

        virtual void Process(const TWatchedEvent& event) = 0;
    };

    class TSessionId {
    public:
        ~TSessionId();

    private:
        friend class TZooKeeper;

        class TImpl;
        THolder<TImpl> Impl;

        TSessionId(TImpl* impl);
    };
    using TSessionIdPtr = TSimpleSharedPtr<TSessionId>;
    using TLogFunction = std::add_pointer<void(const char *message)>::type;

    struct TZooKeeperOptions {
        TZooKeeperOptions(const TString& address)
            : Address(address)
            , Timeout(TDuration::Seconds(5))
            , Watcher(nullptr)
            , ReadOnly(false)
            , LogLevel(LL_ERROR)
            , LogFunction(nullptr)
            , ServersResolutionDelay(TDuration::Seconds(10))
        {
        }

    public:
        TString Address;
        TDuration Timeout;
        IWatcher* Watcher;
        bool ReadOnly;
        ELogLevel LogLevel;
        TLogFunction LogFunction;
        TSessionIdPtr SessionId;
        TDuration ServersResolutionDelay; // setting for zoo_set_servers_resolution_delay
    };

    struct TZooOperation {
        enum EType {
            CreateOp,
            DeleteOp,
            SetOp,
            CheckOp,
        };

        const EType Type;
        const TString Path;

        TZooOperation(EType type, const TString& path)
            : Type(type)
            , Path(path)
        {
        }

        virtual ~TZooOperation() {
        }
    };

    using TZooOperations = TVector<THolder<TZooOperation>>;

    struct TZooCreateOperation: public TZooOperation {
        const TString Data;
        const TVector<TACL> ACL;
        const ECreateMode CreateMode;

        struct ACL_vector ACLVector;

        TZooCreateOperation(const TString& path,
                            const TString& data,
                            const TVector<TACL>& acl,
                            ECreateMode createMode)
            : TZooOperation(TZooOperation::CreateOp, path)
            , Data(data)
            , ACL(acl)
            , CreateMode(createMode)
        {
            ACLVector.count = ACL.size();
            ACLVector.data = const_cast<TACL*>(&ACL.front());
        }
    };

    struct TZooDeleteOperation: public TZooOperation {
        const int Version;

        TZooDeleteOperation(const TString& path, int version)
            : TZooOperation(TZooOperation::DeleteOp, path)
            , Version(version)
        {
        }
    };

    struct TZooSetOperation: public TZooOperation {
        const TString Data;
        const int Version;
        TStatPtr Stat;

        TZooSetOperation(const TString& path, const TString& data, int version = -1)
            : TZooOperation(TZooOperation::SetOp, path)
            , Data(data)
            , Version(version)
            , Stat(MakeHolder<TStat>())
        {
        }
    };

    struct TZooCheckOperation: public TZooOperation {
        const int Version;

        TZooCheckOperation(const TString& path, int version)
            : TZooOperation(TZooOperation::CheckOp, path)
            , Version(version)
        {
        }
    };

    struct TDataCallbackResult {
        std::function<void()> ProcessException;
        TString Data;
        TString Path;
        TStat Stat;
    };

    typedef std::function<void(const TDataCallbackResult& result)> TDataCallback;

    class TZooKeeper {
    public:
        TZooKeeper(const TZooKeeperOptions& options);
        TZooKeeper(const TString& connectString, int sessionTimeout, IWatcher* watcher = nullptr,
                   bool canBeReadOnly = false, const TDuration& serversResolutionDelay = TDuration::Seconds(10));
        TZooKeeper(const TString& connectString, int sessionTimeout, IWatcher* watcher, TSessionIdPtr sessionId,
                   bool canBeReadOnly = false, const TDuration& serversResolutionDelay = TDuration::Seconds(10));

        ~TZooKeeper();

        TSessionIdPtr GetSessionId() const noexcept;

        void Register(IWatcher* watcher) noexcept;
        void Close();

        TString Create(const TString& path, const TString& data, const TVector<TACL>& acl, ECreateMode createMode);
        void CreateHierarchy(const TFsPath& path, const TVector<TACL>& acl);
        void Delete(const TString& path, int version);

        TStatPtr Exists(const TString& path, bool watch = false);
        TVector<TString> GetChildren(const TString& path, bool watch = false);

        TString GetData(const TString& path, bool watch = false, TStat* stat = nullptr);
        void GetData(const TString& path, bool watch, const TDataCallback& cb);

        void SetData(const TString& path, const TString& data, int version = -1);

        void DoTransaction(const TZooOperations& operations);

        NThreading::TFuture<TString> CreateAsync(const TString& path, const TString& data, const TVector<TACL>& acl, ECreateMode createMode);
        NThreading::TFuture<void> DeleteAsync(const TString& path, int version);

        NThreading::TFuture<TStatPtr> ExistsAsync(const TString& path, bool watch = false);
        NThreading::TFuture<TVector<TString>> GetChildrenAsync(const TString& path, bool watch = false);

        NThreading::TFuture<TString> GetDataAsync(const TString& path, bool watch = false);
        NThreading::TFuture<std::pair<TString, TStatPtr>> GetDataWithStatAsync(const TString& path, bool watch = false);

        NThreading::TFuture<void> SetDataAsync(const TString& path, const TString& data, int version = -1);

        NThreading::TFuture<void> DoTransactionAsync(const TZooOperations& operations);

        void Reset(const TZooKeeperOptions& options);
        void Reset(const TString& connectString, int sessionTimeout, IWatcher* watcher = nullptr,
                   bool canBeReadOnly = false, const TDuration& serversResolutionDelay = TDuration::Seconds(10));
        void Reset(const TString& connectString, int sessionTimeout, IWatcher* watcher, TSessionIdPtr sessionId,
                   bool canBeReadOnly = false, const TDuration& serversResolutionDelay = TDuration::Seconds(10));

    protected:
        friend class TSessionId;

        class TImpl;
        THolder<TImpl> Impl;
    };

}
