#pragma once

#include "state.h"

#include <kernel/common_server/rt_background/settings.h>

template <class TDataUsagePolicy>
class TDBTableScannerImpl : public IRTRegularBackgroundProcess {
private:
    using TBase = IRTRegularBackgroundProcess;
    Y_HAS_MEMBER(PrefilterRecordsWithoutLock);
protected:
    class TProcessRecordsContext {
        CSA_DEFAULT(TProcessRecordsContext, ui64, LastEventId);
        CSA_READONLY_FLAG(DryRun, false);
    public:
        TProcessRecordsContext(const ui64 lastEventId, const bool dryRun = false)
            : LastEventId(lastEventId)
            , DryRunFlag(dryRun)
        {}
    };

    virtual bool ProcessRecords(const typename TDataUsagePolicy::TRecords& records, const TExecutionContext& context, TProcessRecordsContext& processContext) const = 0;

    mutable TDataUsagePolicy DataUsagePolicy;
    virtual bool PrepareDataPolicy(const IBaseServer& /*server*/) const {
        return true;
    }
public:
    TString GetDBName() const {
        return DataUsagePolicy.GetDBName();
    }

    TString GetTableName() const {
        return DataUsagePolicy.GetTableName();
    }

    TString GetEventIdColumn() const {
        return DataUsagePolicy.GetEventIdColumn();
    }

    using TBase::TBase;
    virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> stateExt, const TExecutionContext& context) const override {
        auto gLogging = TFLRecords::StartContext().Method("TDBTableScannerImpl::DoExecute")("table_name", DataUsagePolicy.GetTableName());
        if (!PrepareDataPolicy(context.GetServer())) {
            TFLEventLog::Error("cannot prepare data policy");
            return nullptr;
        }
        THolder<TRTHistoryWatcherState> result = THolder(BuildState());

        const TRTHistoryWatcherState* state = dynamic_cast<const TRTHistoryWatcherState*>(stateExt.Get());
        TProcessRecordsContext processContext(state ? state->GetLastEventId() : StartFromId, DryRun);

        TMaybe<ui64> maxEventId;

        NStorage::IDatabase::TPtr dataBase = context.GetServer().GetDatabase(DataUsagePolicy.GetDBName());
        if (!dataBase) {
            return nullptr;
        }
        bool withoutLock = false;
        constexpr bool hasPrefilterWithoutLock = THasPrefilterRecordsWithoutLock<TDataUsagePolicy>::value;
        while (Now() - StartInstant < LockTimeout) {
            NCS::NStorage::TRecordsSetWT records;
            const TString selectRequest = "SELECT MAX(" + DataUsagePolicy.GetEventIdColumn() + ") as max_id FROM " + DataUsagePolicy.GetTableName();
            auto transaction = dataBase->CreateTransaction(false);
            transaction->SetExpectFail(true);
            auto queryResult = transaction->MultiExec({
                    {"LOCK TABLE " + DataUsagePolicy.GetTableName() + " IN SHARE MODE NOWAIT"},
                    {selectRequest, records},
                    {"COMMIT"}
                });

            if (!queryResult) {
                if (hasPrefilterWithoutLock) {
                    TFLEventLog::Notice("Lock failed. Reask with no lock");
                    auto noLockTransaction = dataBase->CreateTransaction(true);
                    auto result = noLockTransaction->Exec(selectRequest, &records);
                    if (!result) {
                        TFLEventLog::Error("Failed get max id with no lock");
                        Sleep(TDuration::Seconds(1));
                        continue;
                    }
                    withoutLock = true;
                } else {
                    TFLEventLog::Notice("try get lock again");
                    Sleep(TDuration::Seconds(1));
                    continue;
                }
            }
            if (!records.size()) {
                TFLEventLog::Error("cannot receive max_id for scanner");
                return nullptr;
            }
            TMaybe<ui64> maxId = records.front().CastTo<ui64>("max_id");
            if (!maxId) {
                TFLEventLog::Error("cannot parse max_id")("raw_data", records.front().SerializeToString());
                return nullptr;
            }
            maxEventId = *maxId;
            break;
        }

        if (!maxEventId.Defined()) {
            TFLEventLog::Error("cannot get lock with deadline");
            return nullptr;
        }

        TFLEventLog::LTSignal("table_scanner_remains_to_process", *maxEventId - processContext.GetLastEventId())("&table_name", DataUsagePolicy.GetTableName());

        typename TDataUsagePolicy::TRecords records;
        {
            auto transaction = dataBase->CreateTransaction(false);
            TStringStream selector;
            selector << "SELECT * FROM " << DataUsagePolicy.GetTableName()
                << " WHERE " << DataUsagePolicy.GetEventIdColumn() << " > " << processContext.GetLastEventId()
                << " AND " << DataUsagePolicy.GetEventIdColumn() << " <= " << maxEventId.GetRef();
            if (Condition) {
                selector << " AND " << '(' << Condition << ')';
            }
            if (Quantum) {
                selector << " ORDER BY " << DataUsagePolicy.GetEventIdColumn();
                selector << " LIMIT " << Quantum;
            }

            auto queryResult = transaction->Exec(selector.Str(), &records);
            if (!queryResult) {
                return nullptr;
            }
        }

        if constexpr (hasPrefilterWithoutLock) {
            if (withoutLock) {
                if (!DataUsagePolicy.PrefilterRecordsWithoutLock(records)) {
                    TFLEventLog::Error("Failed prefilter records");
                }
            }
        }

        const ui64 oldLastEventId = processContext.GetLastEventId();
        TFLEventLog::Info("ProcessRecords info")("last_event_id_before", processContext.GetLastEventId())("records_count", records.size())("dry_run_enabled", processContext.IsDryRun());
        if (!ProcessRecords(records, context, processContext)) {
            TFLEventLog::Error("Failed to run ProcessRecords");
            return nullptr;
        }
        TFLEventLog::Info("ProcessRecords info")("last_event_id_after", processContext.GetLastEventId());
        if (!DryRun) {
            result->SetLastEventId(processContext.GetLastEventId());
        } else {
            result->SetLastEventId(oldLastEventId);
        }
        return result.Release();
    }

    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
        NFrontend::TScheme scheme = TBase::DoGetScheme(server);
        TDataUsagePolicy::FillScheme(scheme, server);
        scheme.Add<TFSBoolean>("dry_run", "Пробный запуск (пишет логи, не сдвигает курсор, не пишет записи)");
        scheme.Add<TFSNumeric>("start_from", "Начать с id").SetDefault(0);
        scheme.Add<TFSString>("condition", "SQL условие на данные");
        scheme.Add<TFSNumeric>("quantum", "Максимальный размер пакета для обработки").SetDefault(0);
        scheme.Add<TFSNumeric>("lock_timeout", "Таймаут на лок (секунды)").SetDefault(TDuration::Minutes(5).Seconds());
        return scheme;
    }

    virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
        if (!TBase::DoDeserializeFromJson(jsonInfo)) {
            return false;
        }
        if (!DataUsagePolicy.DeserializeFromJson(jsonInfo)) {
            return false;
        }
        JREAD_BOOL_OPT(jsonInfo, "dry_run", DryRun);
        JREAD_UINT_OPT(jsonInfo, "start_from", StartFromId);
        JREAD_STRING_OPT(jsonInfo, "condition", Condition);
        JREAD_UINT_OPT(jsonInfo, "quantum", Quantum);
        JREAD_DURATION_OPT(jsonInfo, "lock_timeout", LockTimeout);
        return true;
    }

    virtual NJson::TJsonValue DoSerializeToJson() const override {
        NJson::TJsonValue result = TBase::DoSerializeToJson();
        DataUsagePolicy.SerializeToJson(result);
        JWRITE(result, "dry_run", DryRun);
        JWRITE(result, "start_from", StartFromId);
        JWRITE(result, "condition", Condition);
        JWRITE(result, "quantum", Quantum);
        JWRITE(result, "lock_timeout", LockTimeout.Seconds());
        return result;
    }

private:
    virtual TRTHistoryWatcherState* BuildState() const = 0;
private:
    CS_ACCESS(TDBTableScannerImpl, ui64, StartFromId, 0);
    CSA_DEFAULT(TDBTableScannerImpl, TString, Condition);
    CS_ACCESS(TDBTableScannerImpl, ui32, Quantum, 0);
    CS_ACCESS(TDBTableScannerImpl, TDuration, LockTimeout, TDuration::Minutes(5));
    CS_ACCESS(TDBTableScannerImpl, bool, DryRun, false);
};

namespace NDBTableScanner {

    class TDefaultDBScannerPolicy {
    public:
        static void FillScheme(NFrontend::TScheme& /*scheme*/, const IBaseServer& /*server*/) {
        }

        void SerializeToJson(NJson::TJsonValue& /*result*/) const {
        }

        bool DeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) {
            return true;
        }

        static TString GetEventIdColumn() {
            return "history_event_id";
        }
    };

    class TCommonScannerPolicy {
    public:
        using TRecords = TRecordsSetWT;
        CSA_DEFAULT(TCommonScannerPolicy, TString, TableName);
        CSA_DEFAULT(TCommonScannerPolicy, TString, DBName);
        CS_ACCESS(TCommonScannerPolicy, TString, EventIdColumn, "history_event_id");
    public:
        static void FillScheme(NFrontend::TScheme& scheme, const IBaseServer& server);
        void SerializeToJson(NJson::TJsonValue& result) const;
        bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    };

}

class TDBTableScanner: public TDBTableScannerImpl<NDBTableScanner::TCommonScannerPolicy> {
public:
    TDBTableScanner& SetTableName(const TString& tableName) {
        DataUsagePolicy.SetTableName(tableName);
        return *this;
    }
    TDBTableScanner& SetDBName(const TString& dbName) {
        DataUsagePolicy.SetDBName(dbName);
        return *this;
    }
    TDBTableScanner& SetEventIdColumn(const TString& eventIdColumn) {
        DataUsagePolicy.SetEventIdColumn(eventIdColumn);
        return *this;
    }
};
