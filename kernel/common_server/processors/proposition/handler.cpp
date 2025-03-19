#include "handler.h"
#include <kernel/common_server/proposition/manager.h>

namespace NCS {
    namespace NHandlers {

        using TDBProposition = NPropositions::TDBProposition;

        void TPropositionUpsertHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBProposition::GetTableName() + " manager not configured");

            TVector<TDBProposition> objects;
            ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects container");

            auto session = GetServer().GetPropositionsManager()->BuildNativeSession(false);
            for (auto&& i : objects) {
                ReqCheckCondition(i.CheckPolicy(GetServer()), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, TDBProposition::GetTableName() + " wrong proposition policy configuration");
                bool isUpdate = false;
                if (!GetServer().GetPropositionsManager()->UpsertObject(i, permissions->GetUserId(), session, &isUpdate, nullptr)) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
                if (isUpdate) {
                    TBase::template ReqCheckPermissions<TPropositionPermissions>(permissions, TPropositionPermissions::EObjectAction::Modify, i.GetProposedObject());
                } else {
                    TBase::template ReqCheckPermissions<TPropositionPermissions>(permissions, TPropositionPermissions::EObjectAction::Add, i.GetProposedObject());
                }
            }
            if (!session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }

        void TPropositionRemoveHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBProposition::GetTableName() + " manager not configured");

            TVector<TDBProposition> objects;
            TSet<ui32> propositionIds;
            ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "objects", propositionIds), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "cannot_read_ids");
            ReqCheckCondition(GetServer().GetPropositionsManager()->GetCustomObjects(objects, propositionIds), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "cannot_fetch_ids");
            ReqCheckCondition(objects.size() == propositionIds.size(), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "incorrect_ids");
            for (auto&& i : objects) {
                TBase::template ReqCheckPermissions<TPropositionPermissions>(permissions, TPropositionPermissions::EObjectAction::Remove, i.GetProposedObject());
            }

            auto session = GetServer().GetPropositionsManager()->BuildNativeSession(false);
            session.SetComment("declined");
            if (!GetServer().GetPropositionsManager()->RemoveObject(propositionIds, permissions->GetUserId(), session) || !session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }

        bool TPropositionInfoHandler::DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& server) const {
            auto& rMethod = scheme.Method(NScheme::ERequestMethod::Post);
            rMethod.Body().Content();
            auto& replyScheme = rMethod.Response(HTTP_OK).AddContent();
            replyScheme.Add<TFSArray>("objects").SetElement(TDBProposition::GetScheme(server));
            return true;
        }

        void TPropositionInfoHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBProposition::GetTableName() + " manager not configured");
            TVector<TDBProposition> objects;
            ReqCheckCondition(GetServer().GetPropositionsManager()->GetObjects(objects), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");
            NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
            for (auto&& i : objects) {
                if (!permissions->Check<TPropositionPermissions>(TPropositionPermissions::EObjectAction::Observe, i.GetProposedObject())) {
                    continue;
                }
                objectsJson.AppendValue(i.SerializeToJson());
            }
            g.MutableReport().AddReportElement("objects", std::move(objectsJson));
        }

        bool TPropositionVerdictHandler::DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& /*server*/) const {
            auto& rMethod = scheme.Method(NScheme::ERequestMethod::Post);
            auto& content = rMethod.Body().Content();
            content.Add<TFSVariants>("verdict").InitVariants<NPropositions::EVerdict>();
            content.Add<TFSArray>("objects").SetElement<TFSString>().SetRequired(false);
            rMethod.Response(HTTP_OK).AddContent();
            return true;
        }

        class TFullPropositionId {
        private:
            CS_ACCESS(TFullPropositionId, ui32, PropositionId, 0);
            CS_ACCESS(TFullPropositionId, ui32, Revision, 0);
        public:
            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TJsonProcessor::Read(jsonInfo, "id", PropositionId)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
                    return false;
                }
                return true;
            }
        };

        void TPropositionVerdictHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBProposition::GetTableName() + " manager not configured");
            TVector<NPropositions::TDBVerdict> verdicts;
            ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", verdicts), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "cannot_read_verdicts");
            TMap<ui32, ui32> revisionById;
            TSet<ui32> propositionIds;
            for (auto&& i : verdicts) {
                revisionById.emplace(i.GetPropositionId(), i.GetPropositionRevision());
                propositionIds.emplace(i.GetPropositionId());
            }

            {
                TVector<NPropositions::TDBProposition> objects;
                ReqCheckCondition(GetServer().GetPropositionsManager()->GetCustomObjects(objects, propositionIds), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "cannot_fetch_ids");
                ReqCheckCondition(objects.size() == propositionIds.size(), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "incorrect_ids");
                for (auto&& i : objects) {
                    TBase::template ReqCheckPermissions<TPropositionPermissions>(permissions, TPropositionPermissions::EObjectAction::Confirm, i.GetProposedObject());
                }
                for (auto&& i : objects) {
                    auto it = revisionById.find(i.GetPropositionId());
                    ReqCheckCondition(it != revisionById.end(), ConfigHttpStatus.UserErrorState, "cannot found revision for proposition: " + ::ToString(i.GetPropositionId()));
                    ReqCheckCondition(it->second == i.GetRevision(), ConfigHttpStatus.UserErrorState, "incorrect revision for proposition: " + ::ToString(i.GetPropositionId()));
                }
            }

            for (auto&& i : verdicts) {
                auto session = GetServer().GetPropositionsManager()->BuildNativeSession(false);
                i.SetSystemUserId(permissions->GetUserId());
                if (!GetServer().GetPropositionsManager()->Verdict(i, permissions->GetUserId(), session, TBase::Context) || !session.Commit()) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
        }

        const IDirectObjectsOperator<NPropositions::TDBProposition>* TPropositionHistoryInfoHandler::GetObjectsManager() const {
            return GetServer().GetPropositionsManager();
        }

        const IDirectObjectsOperator<NPropositions::TDBVerdict>* TPropositionVerdictHistoryInfoHandler::GetObjectsManager() const {
            if (!GetServer().GetPropositionsManager()) {
                return nullptr;
            }
            return GetServer().GetPropositionsManager()->GetVerdictManager();
        }

        bool TPropositionVerdictInfoHandler::DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& server) const {
            auto& rMethod = scheme.Method(NScheme::ERequestMethod::Get);
            auto& replyScheme = rMethod.Response(HTTP_OK).AddContent();
            replyScheme.Add<TFSArray>("objects").SetElement(GetExtendedPropositionScheme(server));
            return true;
        }

        void TPropositionVerdictInfoHandler::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            const auto* propManager = GetServer().GetPropositionsManager();
            ReqCheckCondition(propManager, ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBProposition::GetTableName() + " manager not configured");
            TVector<TDBProposition> objects;
            ReqCheckCondition(propManager->GetObjects(objects), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");
            NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
            for (auto&& i : objects) {
                if (!permissions->Check<TPropositionPermissions>(TPropositionPermissions::EObjectAction::Confirm, i.GetProposedObject())) {
                    continue;
                }
                NPropositions::TDBVerdict obj;
                obj.SetPropositionId(i.GetPropositionId());
                obj.SetPropositionRevision(i.GetRevision());
                obj.SetSystemUserId(permissions->GetUserId());
                auto objJson = obj.SerializeToJson();
                objJson["_title"] = i.GetTitle();
                auto session = propManager->BuildNativeSession(true);
                TVector<NPropositions::TDBVerdict> verticts;
                ReqCheckCondition(propManager->GetVerdictManager()->GetVerdicts(i, verticts, session), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");
                for (const auto& v: verticts) {
                    TStringBuilder s;
                    s << v.GetVerdictInstant() << " " << v.GetSystemUserId() << " " << v.GetVerdict() << " " << v.GetComment();
                    objJson["other_verdicts"].AppendValue(s);
                }
                objectsJson.AppendValue(objJson);
            }
            g.MutableReport().AddReportElement("objects", std::move(objectsJson));
        }

        NFrontend::TScheme TPropositionVerdictInfoHandler::GetExtendedPropositionScheme(const IBaseServer& server) {
            auto result = NPropositions::TDBVerdict::GetScheme(server);
            auto& other = result.Add<TFSArray>("other_verdicts", "other verdicts").SetRequired(false);
            other.SetElement<TFSString>().SetWritable(false).SetReadOnly(true);
            other.SetReadOnly(true).SetWritable(false);
            return result;
        }
    }
}

