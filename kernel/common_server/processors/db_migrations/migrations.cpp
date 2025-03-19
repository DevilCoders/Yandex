#include "migrations.h"
#include <kernel/common_server/migrations/manager.h>

namespace NCS {
    namespace NHandlers {
        TDBMigrationsPermissions::TFactory::TRegistrator<TDBMigrationsPermissions> TDBMigrationsPermissions::Registrator(TDBMigrationsPermissions::GetTypeName());

        TString TDBMigrationsPermissions::GetTypeName() {
            return "db_migrations";
        }

        TString TDBMigrationsPermissions::GetClassName() const {
            return GetTypeName();
        }

        bool TDBMigrationsPermissions::Check(const EObjectAction& action) const {
            return GetActions().contains(action);
        }

        void TDBMigrationsInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TDBMigrationsPermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);
            TVector<NCS::NStorage::TDBMigration> migrations;
            for (const auto& dbName: GetServer().GetDatabaseNames()) {
                if (auto manager = GetServer().GetDBMigrationsManager(dbName)) {
                    ReqCheckCondition(manager->GetMigrations(migrations), ConfigHttpStatus.UnknownErrorStatus, "cannot restore migrations");
                }
            }

            NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
            for (auto& migration : migrations) {
                objectsJson.AppendValue(migration.SerializeToJson());
            }

            g.MutableReport().AddReportElement("objects", std::move(objectsJson));
        }

        void TDBMigrationsApplyProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TDBMigrationsPermissions>(permissions, TAdministrativePermissions::EObjectAction::Modify);
            const auto& data = GetJsonData();
            
            ReqCheckCondition(data.Has("objects") && data["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect migrations array");
            for (const auto& migrationJson : data["objects"].GetArray()) {
                NStorage::TDBMigrationAction action;
                ReqCheckCondition(action.DeserializeFromJson(migrationJson), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect migrations array");
                if (auto manager = GetServer().GetDBMigrationsManager(action.GetDBName())) {
                    ReqCheckCondition(manager->ProcessAction(action, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot process migration");
                }
            }
        }

    }
}
