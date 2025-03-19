#include "manager.h"
#include <util/generic/algorithm.h>

namespace NCS {
    namespace NStorage {

        TDBMigrationsManager::TDBMigrationsManager(const IHistoryContext& hContext, const TDBMigrationsConfig& config, const IBaseServer* server)
            : TBase(hContext, config)
            , Config(config)
            , Server(server)
        {
            for (const auto& s: config.GetMigrationSources()) {
                Sources.emplace_back(s->ConstructSource());
            }
        }

        bool TDBMigrationsManager::ProcessAction(const TDBMigrationAction& action, const TString& userId) {
            if (!action.GetMarkApplied() && !action.GetApply()) {
                return true;
            }
            NCS::NStorage::IDatabase::TPtr dbMigrations = GetDatabase();
            auto session = BuildNativeSession(false);
            auto migration = RestoreMigration(action.GetName(), session);
            if (!migration) {
                return false;
            }
            migration->SetRevision(action.GetRevisionMaybe());
            if (action.GetApply() && !migration->Apply(dbMigrations, session, &action.GetHeader())) {
                return false;
            }
            if (action.GetMarkApplied() && !migration->MarkApplied(dbMigrations, session, &action.GetHeader())) {
                return false;
            }
            if (!UpsertObject(*migration, userId, session)) {
                return false;
            }
            return session.Commit();
        }

        bool TDBMigrationsManager::ApplyMigrations() noexcept {
            auto lock = Database->Lock("MIGRATIONS", true, Max<TDuration>());
            auto gLogging = TFLRecords::StartContext().Method("ApplyMigrations");
            TMigrationsMap migrationsFromDb;
            if (!RestoreMigrations(migrationsFromDb)) {
                return false;
            }
            for (const auto& s : Sources) {
                auto migrations = s->CreateMigrations();
                NCS::NStorage::IDatabase::TPtr dbMigrations = GetDatabase();
                if (!!Server) {
                    dbMigrations = Server->GetDatabase(s->GetDBName());
                } else {
                    CHECK_WITH_LOG(s->GetDBName() == Config.GetDBName());
                }
                if (!dbMigrations) {
                    TFLEventLog::Error("incorrect base for migration")("db_name", s->GetDBName());
                    return false;
                }

                for (auto& migration : migrations) {
                    auto gLogging = TFLRecords::StartContext()("migration_name", migration.GetName())("db_name", s->GetDBName());
                    if (const auto* migrationFromDb = MapFindPtr(migrationsFromDb, migration.GetName())) {
                        migration.MergeFrom(*migrationFromDb);
                    }
                    if (migration.GetHeader().GetAutoApply() && !migration.HasAppliedAt()) {
                        auto session = BuildNativeSession(false);
                        if (!migration.Apply(dbMigrations, session)) {
                            if (migration.GetHeader().GetRequired()) {
                                TFLEventLog::Error("required migration failed");
                                return false;
                            }
                        } else {
                            if (!UpsertObject(migration, "root", session)) {
                                return false;
                            }
                            if (!session.Commit()) {
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        NFrontend::TScheme TDBMigrationsManager::GetScheme(const IBaseServer& server) noexcept {
            NFrontend::TScheme result;
            result.Add<TFSArray>("migrations").SetElement(TDBMigration::GetScheme(server));
            result.Add<TFSString>("name").SetReadOnly(true);
            return result;
        }

        bool TDBMigrationsManager::GetMigrations(TVector<TDBMigration>& res) const {
            TMigrationsMap migrationsFromDb;
            if (!RestoreMigrations(migrationsFromDb)) {
                return false;
            }

            for (const auto& s : Sources) {
                auto migrations = s->CreateMigrations();
                for (auto& migration : migrations) {
                    auto i = migrationsFromDb.find(migration.GetName());
                    if (i != migrationsFromDb.end()) {
                        migration.MergeFrom(i->second);
                        migrationsFromDb.erase(i);
                    }
                    migration.SetDBName(Database->GetName());
                    res.emplace_back(std::move(migration));
                }
            }
            for (auto& [name, migration]: migrationsFromDb) {
                migration.SetDBName(Database->GetName());
                res.emplace_back(std::move(migration));
            }

            return true;
        }

        bool TDBMigrationsManager::RestoreMigrations(TMigrationsMap& result) const noexcept {
            if (FindPtr(Database->GetAllTableNames(), TDBMigration::GetTableName())) {
                auto session = BuildNativeSession(true);
                TVector<TDBMigration> migrations;
                if (!RestoreAllObjects(migrations, session)) {
                    return false;
                }
                for (auto&& m : migrations) {
                    result[m.GetName()] = std::move(m);
                }
            }
            return true;
        }

        TMaybe<NCS::NStorage::TDBMigration> TDBMigrationsManager::RestoreMigration(const TString& name, TEntitySession& session) const noexcept {
            TMaybe<NCS::NStorage::TDBMigration> result;
            if (!RestoreObject(name, result, session)) {
                return Nothing();
            }
            for (const auto& s : Sources) {
                auto migrations = s->CreateMigrations();
                for (auto& migration : migrations) {
                    if (migration.GetName() == name) {
                        if (result) {
                            migration.MergeFrom(*result);
                        }
                        return migration;
                    }
                }
            }
            return result;
        }
    }
}
