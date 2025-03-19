#include "process.h"

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/rt_background/processes/common/yt.h>

IRTRegularBackgroundProcess::TFactory::TRegistrator<TDBCleanerProcess> TDBCleanerProcess::Registrator(TDBCleanerProcess::GetTypeName());

NFrontend::TScheme TDBCleanerProcess::DoGetScheme(const IBaseServer& server) const {
    NFrontend::TScheme result = TBase::DoGetScheme(server);
    result.Add<TFSString>("table_name").SetNonEmpty(true);
    result.Add<TFSVariants>("db_name").SetVariants(server.GetDatabaseNames());
    result.Add<TFSString>("primary_key").SetDefault("history_event_id").SetNonEmpty(true);
    result.Add<TFSString>("timestamp_column").SetDefault("history_timestamp").SetNonEmpty(true);
    result.Add<TFSNumeric>("max_age").SetDefault(10 * 3600 * 24);
    result.Add<TFSNumeric>("cleaner_discretization_records_count").SetDefault(10000);
    result.Add<TFSVariants>("notifier_name").SetVariants(server.GetNotifierNames());
    result.Add<TFSNumeric>("pause_between_removes").SetDefault(10);
    result.Add<TFSBoolean>("timestamp_is_integer").SetDefault(true);
    return result;
}

void TDBCleanerProcess::RemoveCommon(const TString& query, const TString& target, const TInstant instantBorder, const IBaseServer& server, const TString& primaryKey, const NStorage::IDatabase::TPtr database) const {
    const ui32 recordsCountLimit = 1000 * CleanerDiscretizationRecordsCount;
    while (true) {
        TRecordsSetWT records;
        {
            auto session = NCS::TEntitySession(database->CreateTransaction(false));
            if (!session.GetTransaction()->Exec(
                "SELECT " + primaryKey + " FROM " + target + " WHERE "
                + query
                + " ORDER BY " + primaryKey + " LIMIT " + ::ToString(recordsCountLimit), &records)->IsSucceed()) {
                IFrontendNotifier::Notify(server.GetNotifier(NotifierName), "Не получилось проверить устаревшие записи для " + target);
                return;
            }
        }

        if (records.empty()) {
            return;
        }

        TVector<TString> idsAll;
        TString maxId;
        for (auto&& item: records) {
            const TString idLocal = item.GetString(primaryKey);
            if (!idLocal) {
                IFrontendNotifier::Notify(server.GetNotifier(NotifierName), "Некорректный id для удаления в " + target + ": " + idLocal);
                TFLEventLog::Error("incorrect primary key value")("id", idLocal);
                return;
            }
            idsAll.emplace_back(idLocal);
            maxId = idLocal;
        }

        IFrontendNotifier::Notify(server.GetNotifier(NotifierName),
            "Планируется удалить из " + target + " : " + ::ToString(records.size()) + " записей пачками по " + ::ToString(CleanerDiscretizationRecordsCount) + " штук. "
            + " Максимальный ID: " + maxId);

        ui32 removedRecords = 0;
        const ui32 maxAtt = 5;
        TString maxIdRemoved;
        for (ui32 i = 0; i < idsAll.size();) {
            const ui32 next = ::Min<ui32>(i + CleanerDiscretizationRecordsCount, idsAll.size());
            TVector<TString> ids;
            TString maxIdRemovedNext;
            for (ui32 j = i; j < next; ++j) {
                ids.emplace_back(idsAll[j]);
                maxIdRemovedNext = idsAll[j];
            }
            TFLEventLog::Info("REMOVE OLD HISTORY")("to_id", maxIdRemovedNext)("from_id", ids.front());
            for (ui32 att = 1; att <= maxAtt; ++att) {
                auto session = NCS::TEntitySession(database->CreateTransaction(false));
                if (!session.GetTransaction()->Exec("DELETE FROM " + target + " WHERE " + primaryKey + " IN (" + session.GetTransaction()->Quote(ids) + ")")->IsSucceed() || !session.Commit()) {
                    if (att == maxAtt) {
                        IFrontendNotifier::Notify(server.GetNotifier(NotifierName),
                            "Не получилось удалить пакет старых записей истории " + target + " целиком: удалено "
                            + ::ToString(removedRecords) + " записей до id: " + maxIdRemoved + " из " + ::ToString(idsAll.size()));
                        return;
                    } else {
                        TFLEventLog::Error("new attemption to delete old records")("att", att);
                        continue;
                    }
                } else {
                    removedRecords += ids.size();
                    break;
                }
            }
            maxIdRemoved = maxIdRemovedNext;
            Sleep(PauseBetweenRemoves);
            i = next;
        }
        IFrontendNotifier::Notify(server.GetNotifier(NotifierName),
            "Удалены старые записи истории из " + target + " до " + instantBorder.ToString() + " (" + ::ToString(removedRecords) + " записей)");

        if (records.size() < recordsCountLimit) {
            break;
        }
    }
}

TAtomicSharedPtr<IRTBackgroundProcessState> TDBCleanerProcess::DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> /*state*/, const TExecutionContext& context) const {
    const IBaseServer& server = context.GetServerAs<IBaseServer>();
    auto database = server.GetDatabase(DBName);
    if (!database) {
        ALERT_NOTIFY << "In DB cleaner, nonexistent database " << DBName << " was requested" << Endl;
        return nullptr;
    }
    auto table = database->GetTable(TableName);
    if (!table) {
        ALERT_NOTIFY << "In DB cleaner, nonexistent table " << TableName << " was requested" << Endl;
        return nullptr;
    }
    const TInstant instantBorder = Now() - MaxAge;
    TString query;
    if (TimestampIsIntegerFlag) {
        query = TimestampColumn + " < " + ToString(instantBorder.Seconds());
    } else {
        query = TimestampColumn + " < " + instantBorder.ToString();
    }
    RemoveCommon(query, TableName, instantBorder, server, PrimaryKey, database);
    return MakeAtomicShared<IRTBackgroundProcessState>();
}
