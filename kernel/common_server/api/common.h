#pragma once

#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/searchserver/simple/http_status_config.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/coded_exception.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/storage/selection/iterator.h>
#include <kernel/common_server/api/history/event.h>

#include <library/cpp/json/json_reader.h>

#include <util/memory/blob.h>
#include <util/generic/cast.h>
#include "history/event.h"

class ILocalization;
const TDuration StandartDataFreshnessInReport = TDuration::Seconds(10);

namespace NEntityTagsManager {
    enum class EEntityType {
        Car = 0 /* "car" */,
        User = 1 /* "user" */,
        Trace = 2 /* "trace" */,
        Area = 3 /* "area" */,
        GeoObject = 4 /* "geo_object" */,
        Undefined = 100 /* "undefined" */,
    };

    enum class EMultiplePerformersPolicy {
        Allow,
        Deny,
    };
}

namespace NCS {
    namespace NStorage {
        class IBaseRecordsSet;
    }
}

namespace NCS {
    class ISessionContext {
    public:
        virtual ~ISessionContext() {}
        virtual void OnAfterCommit() = 0;
    };

    class TInfoEntitySession {
    private:
        TVector<NJson::TJsonValue> Comments;
        CSA_DEFAULT(TInfoEntitySession, TString, OriginatorId);
        CSA_DEFAULT(TInfoEntitySession, TString, LocalizedMessageKey);
        CSA_DEFAULT(TInfoEntitySession, TString, LocalizedTitleKey);
        CS_ACCESS(TInfoEntitySession, ESessionResult, Result, ESessionResult::Success);
        CSA_FLAG(TInfoEntitySession, MultiErrors, false);
    private:
        TAtomic ErrorsInfoCounter = 0;

    public:
        TInfoEntitySession() = default;
        virtual ~TInfoEntitySession() = default;

        ESessionResult DetachResult();
        void DoExceptionOnFail(const THttpStatusManagerConfig& config, const ILocalization* localization = nullptr) const;

        bool HasError() const {
            return AtomicGet(ErrorsInfoCounter);
        }

        void ClearErrors();
        class TSessionInfoWriter: public TFLEventLog::TLogWriterContext, public TMoveOnly {
        private:
            using TBase = TFLEventLog::TLogWriterContext;
            TInfoEntitySession* Owner = nullptr;
        public:
            TSessionInfoWriter(TFLEventLog::TLogWriterContext&& context, TInfoEntitySession& owner);
            TSessionInfoWriter(TFLEventLog::TLogWriterContext&& context);
            ~TSessionInfoWriter();
        };

        TSessionInfoWriter SetErrorInfo(const TString& source, const TString& info, const ESessionResult result = ESessionResult::InternalError);
        TSessionInfoWriter Error(const TString& problemMessage, const ESessionResult result = ESessionResult::InternalError);
        TSessionInfoWriter Error(const TString& problemMessage, const bool isFinal);
        TSessionInfoWriter SystemError(const TString& problemMessage) {
            return Error(problemMessage, ESessionResult::InternalError);
        }
        TSessionInfoWriter UserError(const TString& problemMessage) {
            return Error(problemMessage, ESessionResult::IncorrectRequest);
        }

        TInfoEntitySession& AddComment(const NJson::TJsonValue& comment) {
            Comments.emplace_back(comment);
            return *this;
        }

        TInfoEntitySession& SetComment(const NJson::TJsonValue& comment) {
            Comments = { comment };
            return *this;
        }

        NJson::TJsonValue GetCommentJson() const {
            NJson::TJsonValue result = NJson::JSON_ARRAY;
            for (auto&& i : Comments) {
                result.AppendValue(i);
            }
            return result;
        }

        TString GetComment() const {
            if (Comments.empty()) {
                return "";
            } else {
                return GetCommentJson().GetStringRobust();
            }
        }
    };

    class TEntitySession: public TInfoEntitySession {
    public:
        class IAfterCommitAction {
        public:
            virtual ~IAfterCommitAction() = default;
            using TPtr = TAtomicSharedPtr<IAfterCommitAction>;
            virtual void Execute() noexcept = 0;
        };

        class TSendMessageAfterCommitAction: public IAfterCommitAction {
        private:
            IMessage::TPtr Message;
        public:
            TSendMessageAfterCommitAction(IMessage::TPtr message)
                : Message(message)
            {

            }
            virtual void Execute() noexcept override {
                SendGlobalMessage(*Message);
            }
        };
    private:
        using TBase = TInfoEntitySession;

    private:
        TMap<TString, NRTProc::TAbstractLock::TPtr> LocksManager;
        bool NeedCommit = true;
        NStorage::ITransaction::TPtr Transaction;
        TAtomicSharedPtr<ISessionContext> Context;
        TAtomic CommitFlag = 0;
        TVector<IAfterCommitAction::TPtr> AfterCommitActions;
        CS_ACCESS(TEntitySession, bool, DryRun, false);
    public:
        TEntitySession(NStorage::ITransaction::TPtr transaction, TAtomicSharedPtr<ISessionContext> context = nullptr)
            : Transaction(transaction)
            , Context(context)
        {
        }
        virtual ~TEntitySession();

        template <class TQuery>
        bool ExecQuery(const TQuery& query, NCS::NStorage::IBaseRecordsSet* records = nullptr) {
            if (!Transaction->ExecQuery(query, records)) {
                Error("transaction failed")("transaction_id", Transaction->GetTransactionId());
                return false;
            }
            return true;
        }

        template <class TQuery>
        bool Exec(const TQuery& query) {
            if (!Transaction->Exec(query)) {
                Error("transaction failed")("transaction_id", Transaction->GetTransactionId());
                return false;
            }
            return true;
        }

        bool ExecRequest(const NCS::NStorage::NRequest::INode& query) {
            if (!Transaction->ExecRequest(query)->IsSucceed()) {
                Error("transaction failed", ESessionResult::TransactionProblem)("transaction_id", Transaction->GetTransactionId());
                return false;
            }
            return true;
        }

        bool MultiExecRequests(const TSRRequests& queries);

        TEntitySession& DropNeedCommit() {
            NeedCommit = false;
            return *this;
        }

        template <class T>
        TString Quote(const T& data) const {
            return Transaction->Quote(data);
        }

        bool GetNeedCommit() const {
            return NeedCommit && Transaction->GetStatus() == NStorage::ETransactionStatus::InProgress;
        }

        template <class TContainer>
        TString BuildStringValues(const TContainer& values) const {
            return Transaction->Quote(values);
        }

        virtual bool Commit();
        virtual bool Rollback();

        template <class T>
        T* GetContextAs() const {
            return VerifyDynamicCast<T*>(Context.Get());
        }

        bool Locked(const TString& lockKey) const {
            return LocksManager.contains(lockKey);
        }

        bool AddLock(const TString& lockKey, NRTProc::TAbstractLock::TPtr lock) {
            return LocksManager.emplace(lockKey, lock).second;
        }

        bool HasLock(const TString& lockKey) {
            return LocksManager.contains(lockKey);
        }

        TEntitySession& SetTransaction(NStorage::ITransaction::TPtr transaction) {
            Transaction = transaction;
            return *this;
        }

        bool HasTransaction() const {
            return !!Transaction;
        }

        NStorage::ITransaction::TPtr GetTransaction() const {
            CHECK_WITH_LOG(HasTransaction());
            return Transaction;
        }

        const NStorage::IDatabase& GetDatabase() const {
            CHECK_WITH_LOG(HasTransaction());
            return Transaction->GetDatabase();
        }

        void AddAfterCommitAction(IAfterCommitAction::TPtr action) {
            AfterCommitActions.push_back(action);
        }
    };
}

template <class T>
class IPropositionsManager;

template <class TEntity>
class IDirectObjectsOperator {
protected:
    virtual bool DoRestoreHistoryEvent(const ui64 historyEventId, TMaybe<TObjectEvent<TEntity>>& result, NCS::TEntitySession& session) const = 0;
    virtual bool DoRestoreHistory(TVector<TObjectEvent<TEntity>>& result, NCS::TEntitySession& session) const = 0;
    virtual bool DoUpsertObject(const TEntity& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate, TEntity* result) const = 0;
    virtual bool DoUpsertObjectForce(const TEntity& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate, TEntity* result) const = 0;
    virtual bool DoDirectRestoreAllObjects(TVector<TEntity>& result, NCS::TEntitySession& session) const = 0;
    virtual bool DoDirectRestoreObjects(const TSet<typename TEntity::TId>& ids, TVector<TEntity>& result, NCS::TEntitySession& session) const = 0;

public:
    template <class TSelection>
    NCS::NSelection::TReader<TEntity, TSelection> BuildSequentialReader() const {
        return NCS::NSelection::TReader<TEntity, TSelection>(TEntity::GetTableName());
    }

    template <class TSelection>
    typename NCS::NSelection::IObjectsReader<TEntity>::TPtr BuildSequentialReaderPtr() const {
        return new NCS::NSelection::TReader<TEntity, TSelection>(TEntity::GetTableName());
    }

    template <class TSelection>
    NCS::NSelection::TReader<TObjectEvent<TEntity>, TSelection> BuildHistorySequentialReader() const {
        return NCS::NSelection::TReader<TObjectEvent<TEntity>, TSelection>(TEntity::GetTableName() + "_history");
    }

    virtual NCS::TEntitySession BuildNativeSession(const bool readOnly, const bool repeatableRead = false) const = 0;

    bool RestoreHistoryEvent(const ui64 historyEventId, TMaybe<TObjectEvent<TEntity>>& result, NCS::TEntitySession& session) const {
        return DoRestoreHistoryEvent(historyEventId, result, session);
    }

    bool DirectRestoreAllObjects(TVector<TEntity>& result, NCS::TEntitySession& session) const {
        return DoDirectRestoreAllObjects(result, session);
    }

    bool DirectRestoreObjects(const TSet<typename TEntity::TId>& ids, TVector<TEntity>& result, NCS::TEntitySession& session) const {
        return DoDirectRestoreObjects(ids, result, session);
    }

    bool UpsertObject(const TEntity& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate = nullptr, TEntity* result = nullptr) const {
        return DoUpsertObject(object, userId, session, isUpdate, result);
    }

    bool RestoreHistory(TVector<TObjectEvent<TEntity>>& result, NCS::TEntitySession& session) const {
        return DoRestoreHistory(result, session);
    }

    bool UpsertObjectForce(const TEntity& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate = nullptr, TEntity* result = nullptr) const {
        return DoUpsertObjectForce(object, userId, session, isUpdate, result);
    }

    bool RestoreHistoryEventAsCurrent(const ui64 historyEventId, TMaybe<TEntity>& result, const TString& userId, NCS::TEntitySession& session) const {
        TMaybe<TObjectEvent<TEntity>> resultEvent;
        if (!RestoreHistoryEvent(historyEventId, resultEvent, session)) {
            return false;
        }
        if (!resultEvent) {
            result.Clear();
            return true;
        }
        TEntity object;
        if (!UpsertObjectForce(*resultEvent, userId, session, nullptr, &object)) {
            return false;
        }
        result = std::move(object);
        return true;
    }

};

template <class TObjectContainer>
class IDBEntitiesManager: public virtual IDirectObjectsOperator<TObjectContainer> {
public:
    virtual bool RemoveObject(const TSet<typename TObjectContainer::TId>& ids, const TString& userId, NCS::TEntitySession& session) const = 0;
    virtual bool RemoveObject(const typename TObjectContainer::TId& id, const ui32 revision, const TString& userId, NCS::TEntitySession& session) const = 0;
    virtual bool ForceUpsertObject(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session, NStorage::TObjectRecordsSet<TObjectContainer>* containerExt = nullptr, bool* isUpdateExt = nullptr) const = 0;
    virtual bool GetObjects(TMap<typename TObjectContainer::TId, TObjectContainer>& objects, const TInstant reqActuality = TInstant::Zero()) const = 0;
    virtual bool GetObjects(TVector<TObjectContainer>& objects) const = 0;
    virtual bool GetCustomEntities(TVector<TObjectContainer>& objects, const TSet<typename TObjectContainer::TId>& ids) const = 0;
    virtual bool AddObjects(const TVector<TObjectContainer>& objects, const TString& userId, NCS::TEntitySession& session, NStorage::TObjectRecordsSet<TObjectContainer>* containerExt = nullptr) const = 0;
};

template <class TObjectContainer>
class IDBEntitiesWithPropositionsManager: public virtual IDBEntitiesManager<TObjectContainer> {
public:
    virtual const IPropositionsManager<TObjectContainer>* GetPropositions() const = 0;
};
