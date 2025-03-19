#include "handler.h"
#include <kernel/common_server/obfuscator/manager.h>

namespace NCS {
    namespace NHandlers {
        using TDBObfuscator = NObfuscator::TDBObfuscator;

        TObfuscatorPermissions::TFactory::TRegistrator<TObfuscatorPermissions> TObfuscatorPermissions::Registrator(TObfuscatorPermissions::GetTypeName());

        const IDBEntitiesManager<TDBObfuscator>* TObfuscatorInfoProcessor::GetObjectsManager() const {
            return GetServer().GetObfuscatorManager().GetAs<NObfuscator::TDBManager>();
        }

        const IDBEntitiesManager<TDBObfuscator>* TObfuscatorUpsertProcessor::GetObjectsManager() const {
            return GetServer().GetObfuscatorManager().GetAs<NObfuscator::TDBManager>();
        }

        const IDBEntitiesManager<TDBObfuscator>* TObfuscatorRemoveProcessor::GetObjectsManager() const {
            return GetServer().GetObfuscatorManager().GetAs<NObfuscator::TDBManager>();
        }

        void TObfuscatorDebugProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckCondition(GetJsonData()["obfuscator_id"].IsUInteger(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "obfuscator id is not uint");
            ReqCheckCondition(GetJsonData()["message"].IsString(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "message is not string");
            const ui32 id = GetJsonData()["obfuscator_id"].GetUInteger();
            const TString message = GetJsonData()["message"].GetString();
            auto obfuscatorManager = GetServer().GetObfuscatorManager().GetAs<NObfuscator::TDBManager>();
            ReqCheckCondition(!!obfuscatorManager, ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "can not get obfuscator manager");
            TVector<TDBObfuscator> objects;
            ReqCheckCondition(obfuscatorManager->GetCustomEntities(objects, {id}), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "can not fetch obfuscator by id");
            ReqCheckCondition(objects.size() == 1, ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "obfuscator not found");
            ReqCheckPermissions<TObfuscatorPermissions>(permissions, objects.front().GetName(), TAdministrativePermissions::EObjectAction::Observe);
            const auto obfuscated = objects.front()->Obfuscate(message);
            g.AddReportElement("original_message", NJson::TJsonValue(message));
            g.AddReportElement("obfuscated_message", NJson::TJsonValue(obfuscated));
        }

    }
}
