#pragma once

#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/util/accessor.h>

class TDBCleanerProcess: public IRTRegularBackgroundProcess {
    using TBase = IRTRegularBackgroundProcess;
    static TFactory::TRegistrator<TDBCleanerProcess> Registrator;

protected:
    virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;

    virtual void RemoveCommon(const TString& query, const TString& target, const TInstant instantBorder, const IBaseServer& server, const TString& primaryKey, const NStorage::IDatabase::TPtr database) const;
    virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const override;

    virtual NJson::TJsonValue DoSerializeToJson() const override {
        auto result = TBase::DoSerializeToJson();
        result["table_name"] = TableName;
        result["db_name"] = DBName;
        result["primary_key"] = PrimaryKey;
        result["timestamp_column"] = TimestampColumn;
        JWRITE_DURATION(result, "max_age", MaxAge);
        result["cleaner_discretization_records_count"] = CleanerDiscretizationRecordsCount;
        result["notifier_name"] = NotifierName;
        JWRITE_DURATION(result, "pause_between_removes", PauseBetweenRemoves);
        result["timestamp_is_integer"] = TimestampIsIntegerFlag;
        return result;
    }

    virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
        JREAD_STRING(jsonInfo, "table_name", TableName);
        JREAD_STRING(jsonInfo, "db_name", DBName);
        JREAD_STRING_OPT(jsonInfo, "primary_key", PrimaryKey);
        JREAD_STRING(jsonInfo, "timestamp_column", TimestampColumn);
        JREAD_DURATION(jsonInfo, "max_age", MaxAge);
        JREAD_UINT_OPT(jsonInfo, "cleaner_discretization_records_count", CleanerDiscretizationRecordsCount);
        JREAD_STRING_OPT(jsonInfo, "notifier_name", NotifierName);
        JREAD_DURATION_OPT(jsonInfo, "pause_between_removes", PauseBetweenRemoves);
        JREAD_BOOL_OPT(jsonInfo, "timestamp_is_integer", TimestampIsIntegerFlag);
        return TBase::DoDeserializeFromJson(jsonInfo);
    }

private:
    CSA_DEFAULT(TDBCleanerProcess, TString, TableName);
    CSA_DEFAULT(TDBCleanerProcess, TString, DBName);
    CS_ACCESS(TDBCleanerProcess, TString, PrimaryKey, "history_event_id");
    CS_ACCESS(TDBCleanerProcess, TString, TimestampColumn, "history_timestamp");
    CS_ACCESS(TDBCleanerProcess, TDuration, MaxAge, TDuration::Days(90));
    CS_ACCESS(TDBCleanerProcess, size_t, CleanerDiscretizationRecordsCount, 500);
    CSA_DEFAULT(TDBCleanerProcess, TString, NotifierName);
    CS_ACCESS(TDBCleanerProcess, TDuration, PauseBetweenRemoves, TDuration::MilliSeconds(100));
    CSA_FLAG(TDBCleanerProcess, TimestampIsInteger, true);

public:
    static TString GetTypeName() {
        return "db_cleaner";
    }

    virtual TString GetType() const override {
        return GetTypeName();
    }
};
