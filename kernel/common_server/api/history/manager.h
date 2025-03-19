#pragma once
#include "event.h"
#include "session.h"
#include "sequential.h"
#include <kernel/common_server/library/unistat/cache.h>

template <class T>
class THistoryTableRecordSerialized {
public:
    template <class TObject>
    static NStorage::TTableRecord SerializeToTableRecord(const TObject& object) {
        return object.SerializeToTableRecord();
    }
};

template <class TObject, class TBaseImpl>
class TWriteOnlyHistoryManager: public TBaseImpl {
private:
    using TBase = TBaseImpl;

public:
    using TEntity = TObject;
    enum class ESessionState {
        InProgress,
        Finished
    };
    class ISessionWatcher {
    public:
        virtual ~ISessionWatcher() = default;
        virtual ESessionState CheckSessionState(const TObjectEvent<TObject>& ev) const = 0;
        virtual TString GetSessionOwnerColumnName() const = 0;
        virtual TString GetCurrentSessionsTableName() const = 0;
        virtual const TString& GetSessionOwnerId(const TObjectEvent<TObject>& ev) const = 0;
    };

private:
    const TString TableName;
protected:
    virtual const ISessionWatcher* GetSessionWatcher() const {
        return nullptr;
    }
public:

    virtual ~TWriteOnlyHistoryManager() = default;

    using TBase::TBase;

    const TString& GetTableName() const {
        return TableName;
    }

    TWriteOnlyHistoryManager(NStorage::IDatabase::TPtr database, const TString& tableName, const THistoryConfig& config)
        : TBaseImpl(database, tableName, config)
        , TableName(tableName) {
    }

    TWriteOnlyHistoryManager(const IHistoryContext& context, const TString& tableName, const THistoryConfig& config)
        : TBaseImpl(context, tableName, config)
        , TableName(tableName) {
    }

    TWriteOnlyHistoryManager(const IHistoryContext& context, const TString& tableName)
        : TBaseImpl(context.GetDatabase())
        , TableName(tableName) {
    }

    TWriteOnlyHistoryManager(NStorage::IDatabase::TPtr database, const TString& tableName)
        : TBaseImpl(database)
        , TableName(tableName) {
    }

    bool RemoveIds(const TSet<ui64>& recordIds, NCS::TEntitySession& session) const {
        if (recordIds.empty()) {
            return true;
        }
        TSRDelete reqDelete(GetTableName());
        reqDelete.InitCondition<TSRBinary>("history_event_id", recordIds);
        return session.ExecRequest(reqDelete);
    }

    bool AddHistory(const TVector<TObjectEvent<TObject>>& objects, NCS::TEntitySession& session) const {
        auto gLogging = TFLRecords::StartContext().Method("AddHistory")("stage", "add_history_rows");
        if (objects.empty()) {
            return true;
        }
        TRecordsSet historyRecords;
        for (auto&& i : objects) {
            if (!i.GetHistoryUserId()) {
                session.Error("Operator UserId empty", ESessionResult::InconsistencySystem);
                return false;
            }
            historyRecords.AddRow(THistoryTableRecordSerialized<TObject>::SerializeToTableRecord(i));
        }
        auto transaction = session.GetTransaction();
        if (historyRecords.size()) {
            auto historyTable = TBase::Database->GetTable(GetTableName());
            if (!historyTable->AddRows(historyRecords, transaction, "")) {
                return false;
            }
        }
        TCSSignals::SignalAdd("history_manager-" + GetTableName(), "add_records", objects.size());
        return true;
    }

    template <class T>
    bool AddHistory(const TVector<T>& objects, const TString& userId, const EObjectHistoryAction action, NCS::TEntitySession& session) const {
        return AddHistory(TConstArrayRef<TObject>(objects.begin(), objects.end()), userId, action, session);
    }

    bool AddHistory(const TConstArrayRef<TObject>& objects, const TString& userId, const EObjectHistoryAction action, NCS::TEntitySession& session) const {
        TVector<TObjectEvent<TObject>> events;
        TInstant now = ModelingNow();
        for (auto&& i : objects) {
            TObjectEvent<TObject> objEvent(i, action, now, userId, session.GetOriginatorId(), session.GetComment());
            events.emplace_back(std::move(objEvent));
        }
        return AddHistory(events, session);
    }

    bool AddHistory(const TObject& object, const TString& userId, const EObjectHistoryAction action, NCS::TEntitySession& session) const {
        return AddHistory(TVector<TObject>({object}), userId, action, session);
    }
};

template <class TObject, class TBaseImpl>
using TAbstractHistoryManagerImpl = TWriteOnlyHistoryManager<TObject, TBaseImpl>;

template <class T>
using TAbstractHistoryManager = TAbstractHistoryManagerImpl<T, TSequentialTableWithSessions<TObjectEvent<T>>>;

template <class T, class TObjectId = TStringBuf, class TId = TStringBuf>
using TIndexedAbstractHistoryManager = TAbstractHistoryManagerImpl<T, TIndexedSequentialTableImpl<TObjectEvent<T>>>;

template <class TW, class TR, class TObjectId = TStringBuf, class TId = TStringBuf>
using TAsymmetrySimpleAbstractHistoryManager = TAbstractHistoryManagerImpl<TW, TIndexedSequentialTableImpl<TObjectEvent<TR>>>;
