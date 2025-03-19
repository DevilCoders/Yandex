#include "description.h"

#include <kernel/common_server/util/algorithm/container.h>

namespace NCS {

    namespace NHandlers {

        IItemPermissions::TFactory::TRegistrator<TTagDescriptionPermissions> TTagDescriptionPermissions::Registrator(TTagDescriptionPermissions::GetTypeName());

        void TTagDescriptionsListProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TTagDescriptionPermissions>(permissions, ETagDescriptionActions::Observe);

            const TTagDescriptionsManager& manager = GetServer().GetTagDescriptionsManager();
            const TSet<TString> ids = MakeSet(GetStrings(Context->GetCgiParameters(), "ids", false));
            TVector<TDBTagDescription> descriptions;
            auto session = manager.BuildNativeSession(false);
            if (!Context->GetCgiParameters().Has("ids")) {
                if (!manager.RestoreAllObjects(descriptions, session)) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            } else {
                if (!manager.RestoreObjects(ids, descriptions, session)) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
            NJson::TJsonValue tagDescriptionsJson(NJson::JSON_ARRAY);
            for (auto&& i : descriptions) {
                if (i.GetDeprecated() && !permissions->Check<TTagDescriptionPermissions>(ETagDescriptionActions::ObserveDeprecated)) {
                    continue;
                }
                tagDescriptionsJson.AppendValue(i.SerializeToJson());
            }

            auto reportCopy = tagDescriptionsJson;
            g.MutableReport().AddReportElement("tag_descriptions", std::move(reportCopy));
            g.MutableReport().AddReportElement("objects", std::move(tagDescriptionsJson));
        }


        void TTagDescriptionsUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TTagDescriptionPermissions>(permissions, ETagDescriptionActions::Modify);

            const TTagDescriptionsManager& manager = GetServer().GetTagDescriptionsManager();
            TVector<TDBTagDescription> descriptions;
            ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", descriptions, true), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_tag_description");
            auto session = manager.BuildNativeSession(false);
            if (!manager.UpsertTagDescriptions(descriptions, permissions->GetUserId(), session) || !session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }

        void TTagDescriptionsRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TTagDescriptionPermissions>(permissions, ETagDescriptionActions::Remove);
            const TSet<TString> ids = MakeSet(GetStrings(GetJsonData(), { "ids" , "objects" }, false, true));

            const TTagDescriptionsManager& manager = GetServer().GetTagDescriptionsManager();
            auto session = manager.BuildNativeSession(false);
            if (!manager.RemoveTagDescriptions(ids, permissions->GetUserId(), permissions->Check<TTagDescriptionPermissions>(ETagDescriptionActions::RemoveDeprecated), session) || !session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }
    }
}
