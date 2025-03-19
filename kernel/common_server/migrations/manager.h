#pragma once

#include "config.h"
#include "migration.h"
#include <kernel/common_server/library/storage/abstract/config.h>
#include <kernel/common_server/api/history/cache.h>
#include <kernel/common_server/api/history/db_entities.h>

namespace NCS {
    namespace NStorage {

        class TDBMigrationsManager: public TDBEntitiesManager<TDBMigration> {
            using TBase = TDBEntitiesManager<TDBMigration>;
        public:
            using TPtr = TAtomicSharedPtr<TDBMigrationsManager>;
            TDBMigrationsManager(const IHistoryContext& hContext, const TDBMigrationsConfig& config, const IBaseServer* server);

            bool ProcessAction(const TDBMigrationAction& action, const TString& userId);
            bool ApplyMigrations() noexcept;
            static NFrontend::TScheme GetScheme(const IBaseServer& server) noexcept;
            bool GetMigrations(TVector<TDBMigration>& result) const;
        private:
            using TMigrationsMap = TMap<TString, NCS::NStorage::TDBMigration>;
            bool RestoreMigrations(TMigrationsMap& result) const noexcept;
            TMaybe<TDBMigration> RestoreMigration(const TString& name, TEntitySession& session) const noexcept;
            TVector<IDBMigrationsSource::TPtr> Sources;
            const TDBMigrationsConfig Config;
            const IBaseServer* Server = nullptr;
        };
    }
}

