#pragma once

#include "abstract.h"

#include <kernel/daemon/messages.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>
#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/array_ref.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/system/type_name.h>
#include <util/string/hex.h>
#include "abstract/config.h"
#include "abstract/database.h"
#include "abstract/transaction.h"
#include "abstract/query_result.h"
#include "reply/parsed.h"
#include "reply/decoder.h"
#include "reply/abstract.h"
#include "records/set.h"
#include "abstract/table.h"
#include "records/abstract.h"
#include "abstract/lock.h"

class IHistoryContext;

class TDatabaseNamespace {
    using TDBNamespaces = TMap<TString, TString>;
    CSA_DEFAULT(TDatabaseNamespace, TDBNamespaces, OverridenNamespaces);
public:
    bool HasCustomNamespace(const TString& dbName) const {
        const TString ns = GetNamespace(dbName, "");
        return ns != "public" && ns != "";
    }

    TString GetNamespace(const TString& dbName, const TString& external) const {
        auto it = OverridenNamespaces.find(dbName);
        if (it == OverridenNamespaces.end()) {
            return external;
        } else {
            return it->second;
        }
    }

    void SetNamespace(const TString& dbName, const TString& ns) {
        OverridenNamespaces[dbName] = ns;
    }

    TString ToString() const {
        TStringBuilder ss;
        for (auto&& i : OverridenNamespaces) {
            if (!ss.empty()) {
                ss << ", ";
            }
            ss << i.first << ": " << i.second;
        }
        return ss;
    }
};
using IOriginalContainer = NCS::NStorage::IOriginalContainer;
using TBaseDecoder = NCS::NStorage::TBaseDecoder;
using TRecordsSet = NCS::NStorage::TRecordsSet;
using TRecordsSetWT = NCS::NStorage::TRecordsSetWT;

namespace NRTProc {
    using TFakeLock = NCS::NStorage::TFakeLock;
    using TAbstractLock = NCS::NStorage::TAbstractLock;
}

namespace NStorage {
    using ITransaction = NCS::NStorage::ITransaction;
    using IDatabase = NCS::NStorage::IDatabase;
    using TTableRecord = NCS::NStorage::TTableRecord;
    using IDatabaseConfig = NCS::NStorage::IDatabaseConfig;
    using IBaseRecordsSet = NCS::NStorage::IBaseRecordsSet;
    using TPackedRecordsSet = NCS::NStorage::TPackedRecordsSet;
    using IQueryResult = NCS::NStorage::IQueryResult;
    using ITableAccessor = NCS::NStorage::ITableAccessor;
    using TPanicOnFailPolicy = NCS::NStorage::TPanicOnFailPolicy;
    using TDatabaseConfig = NCS::NStorage::TDatabaseConfig;
    using IFutureQueryResult = NCS::NStorage::IFutureQueryResult;
    using IPackedRecordsSet = NCS::NStorage::IPackedRecordsSet;
    using IRecordsSetWT = NCS::NStorage::IRecordsSetWT;
    using TQueryResult = NCS::NStorage::TQueryResult;
    using TSimpleFutureQueryResult = NCS::NStorage::TSimpleFutureQueryResult;
    using TFailedTransaction = NCS::NStorage::TFailedTransaction;
    using IOriginalContainer = NCS::NStorage::IOriginalContainer;

    template <class T, class TContext = TNull>
    using TObjectRecordsSet = NCS::NStorage::TObjectRecordsSet<T, TContext>;

}

using TTransactionPtr = NCS::NStorage::ITransaction::TPtr;
using TDatabasePtr = NCS::NStorage::IDatabase::TPtr;
using TDatabaseConfigPtr = NCS::NStorage::IDatabaseConfig::TPtr;
using TQueryResultPtr = NCS::NStorage::IQueryResult::TPtr;

class TDBEventLogGuard {
private:
    NStorage::IDatabase::TPtr Database;
    const TString EventDescription;
    const TInstant Start = Now();
    TCollectDaemonInfoMessage CDIMessage;
    const TString TableName;
    bool Active = true;
public:

    static void SetActive(const bool activeFlag);

    TDBEventLogGuard(NStorage::IDatabase::TPtr database, const TString& eventDescription, const TString& tableName = "db_event_log");

    ~TDBEventLogGuard();
};
