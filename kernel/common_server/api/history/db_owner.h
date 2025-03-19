#pragma once
#include <kernel/common_server/api/common.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/storage/structured.h>
#include "config.h"

namespace NCS {
    class TDatabaseOwner {
    protected:
        NCS::NStorage::IDatabase::TPtr Database;
    public:
        virtual ~TDatabaseOwner() = default;

        TDatabaseOwner(NCS::NStorage::IDatabase::TPtr db)
            : Database(db) {
            AssertCorrectConfig(!!Database, "Incorrect database");
        }

        NStorage::IDatabase::TPtr GetDatabase() const {
            return Database;
        }

        NCS::TEntitySession BuildNativeSession(const bool readOnly = false, const bool repeatableRead = false) const {
            return NCS::TEntitySession(Database->TransactionMaker().ReadOnly(readOnly).RepeatableRead(repeatableRead).Build());
        }

        THolder<NCS::TEntitySession> BuildNativeSessionPtr(const bool readOnly = false, const bool repeatableRead = false) const {
            return MakeHolder<NCS::TEntitySession>(Database->TransactionMaker().ReadOnly(readOnly).RepeatableRead(repeatableRead).Build());
        }
    };

    template <class THistoryManagerExternal>
    class TDBHistoryOwner: public TDatabaseOwner {
    private:
        using TBase = TDatabaseOwner;
    public:
        using THistoryManager = THistoryManagerExternal;
        using TEntity = typename THistoryManager::TEntity;
    protected:
        const THistoryConfig HistoryConfig;
        THolder<THistoryContext> HistoryContext;
        THolder<THistoryManager> HistoryManager;
    public:
        virtual ~TDBHistoryOwner() = default;

        TDBHistoryOwner(NCS::NStorage::IDatabase::TPtr db, const THistoryConfig& hConfig)
            : TBase(db)
            , HistoryConfig(hConfig)
            , HistoryContext(MakeHolder<THistoryContext>(db))
            , HistoryManager(MakeHolder<THistoryManager>(*HistoryContext, HistoryConfig))
        {
        }

        THistoryManager& GetHistoryManager() {
            return *HistoryManager;
        }
        const THistoryManager& GetHistoryManager() const {
            return *HistoryManager;
        }

        const THistoryConfig& GetHistoryConfig() const {
            return HistoryConfig;
        }
    };
}
