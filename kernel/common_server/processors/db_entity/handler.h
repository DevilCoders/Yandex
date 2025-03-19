#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/proposition/manager.h>
#include <kernel/common_server/proposition/actions/db_entity.h>
#include <kernel/common_server/processors/proposition/permission.h>
#include <library/cpp/regex/pcre/regexp.h>

namespace NCS {

    namespace NHandlers {

        class TObjectOperator {
        private:
            Y_HAS_MEMBER(GetPublicObjectId);
        public:

            template <class TObject>
            static TString GetPublicObjectId(const TObject& object) {
                if constexpr (THasGetPublicObjectId<TObject>::value) {
                    return object.GetPublicObjectId();
                } else {
                    return object.GetInternalId();
                }

            }
        };

        template <class TDBObject>
        class TDBObjectActivitiesDetector {
        public:
            using EAction = NCommonAdministrative::EObjectAction;
            using TActions = ui64;
        };

        template <class TDBObject>
        class TDBObjectPermissions: public TAdministrativePermissionsImpl<typename TDBObjectActivitiesDetector<TDBObject>::EAction, typename TDBObjectActivitiesDetector<TDBObject>::TActions> {
        private:
            using TBase = TAdministrativePermissionsImpl<typename TDBObjectActivitiesDetector<TDBObject>::EAction, typename TDBObjectActivitiesDetector<TDBObject>::TActions>;
            CSA_READONLY_DEF(TString, MatchPattern);
            THolder<TRegExMatch> RegExMatch;
        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
                NFrontend::TScheme result = TBase::DoGetScheme(server);
                result.Add<TFSString>("match_pattern");
                return result;
            }

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                TJsonProcessor::Write(result, "match_pattern", MatchPattern);
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
                if (!TBase::DeserializeFromJson(info)) {
                    return false;
                }
                if (!TJsonProcessor::Read(info, "match_pattern", MatchPattern)) {
                    return false;
                }
                if (!!MatchPattern) {
                    try {
                        RegExMatch.Reset(new TRegExMatch(MatchPattern));
                    } catch (...) {
                        TFLEventLog::Error("cannot compile regex")("pattern", MatchPattern);
                        return false;
                    }
                }
                return true;
            }
        public:
            static TString GetTypeName() {
                return TDBObject::GetTypeName();
            }

            virtual TString GetClassName() const override {
                return TDBObject::GetTypeName();
            }

            bool Check(const TString& objectId, const typename TBase::EObjectAction action) const {
                if (!!MatchPattern) {
                    if (!RegExMatch) {
                        return false;
                    }
                    if (!RegExMatch->Match(objectId.data())) {
                        return false;
                    }
                }
                return TBase::GetActions().contains(action);
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsInfo: public TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDBEntitiesManager<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override {
                ReqCheckCondition(!!GetObjectsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBObject::GetTableName() + " manager not configured");
                TSet<typename TDBObject::TId> ids;
                const bool idsUsage = GetJsonData().Has("object_ids");
                ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "object_ids", ids), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_ids");

                TVector<TDBObject> objects;
                if (!idsUsage) {
                    ReqCheckCondition(GetObjectsManager()->GetObjects(objects), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");
                } else {
                    ReqCheckCondition(GetObjectsManager()->GetCustomEntities(objects, ids), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");

                }
                NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
                for (auto&& i : objects) {
                    if (!permissions->Check<TDBObjectPermissions<TDBObject>>(TObjectOperator::GetPublicObjectId(i), TDBObjectPermissions<TDBObject>::EObjectAction::Observe)) {
                        continue;
                    }
                    objectsJson.AppendValue(i.SerializeToJson());
                }
                g.MutableReport().AddReportElement("objects", std::move(objectsJson));
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBMetaObjectsInfo: public TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDirectObjectsOperator<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override {
                ReqCheckCondition(!!GetObjectsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBObject::GetTableName() + " manager not configured");
                TSet<typename TDBObject::TId> ids;
                const bool idsUsage = GetJsonData().Has("object_ids");
                ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "object_ids", ids), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_ids");
                TVector<TDBObject> objects;
                auto session = GetObjectsManager()->BuildNativeSession(false);
                if (!idsUsage) {
                    ReqCheckCondition(GetObjectsManager()->DirectRestoreAllObjects(objects, session), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");
                } else {
                    ReqCheckCondition(GetObjectsManager()->DirectRestoreObjects(ids, objects, session), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");
                }
                TVector<const TDBObject*> objectsSort;
                for (auto&& i : objects) {
                    if (!permissions->Check<TDBObjectPermissions<TDBObject>>(TObjectOperator::GetPublicObjectId(i), TDBObjectPermissions<TDBObject>::EObjectAction::Observe)) {
                        continue;
                    }
                    objectsSort.emplace_back(&i);
                }
                const auto pred = [](const TDBObject* l, const TDBObject* r) {
                    return TObjectOperator::GetPublicObjectId(*l) < TObjectOperator::GetPublicObjectId(*r);
                };
                std::sort(objectsSort.begin(), objectsSort.end(), pred);
                NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
                for (auto&& i : objectsSort) {
                    objectsJson.AppendValue(i->SerializeToJson());
                }
                g.MutableReport().AddReportElement("objects", std::move(objectsJson));
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsUpsert: public TCommonUpsert<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonUpsert<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDBEntitiesManager<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                ReqCheckCondition(!!GetObjectsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBObject::GetTableName() + " manager not configured");

                TVector<TDBObject> objects;
                ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects container");

                auto session = GetObjectsManager()->BuildNativeSession(false);
                for (auto&& i : objects) {
                    bool isUpdate = false;
                    if (!GetObjectsManager()->UpsertObject(i, permissions->GetUserId(), session, &isUpdate, nullptr)) {
                        session.DoExceptionOnFail(ConfigHttpStatus);
                    }
                    if (isUpdate) {
                        TBase::template ReqCheckPermissions<TDBObjectPermissions<TDBObject>>(permissions, TObjectOperator::GetPublicObjectId(i), TDBObjectPermissions<TDBObject>::EObjectAction::Modify);
                    } else {
                        TBase::template ReqCheckPermissions<TDBObjectPermissions<TDBObject>>(permissions, TObjectOperator::GetPublicObjectId(i), TDBObjectPermissions<TDBObject>::EObjectAction::Add);
                    }
                }
                if (!session.Commit()) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsProposeUpsert: public TCommonUpsert<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonUpsert<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDBEntitiesManager<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
            using TBase::GetServer;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return TDBObject::GetTypeName() + "-propose-upsert";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "propositions manager not configured");
                ReqCheckCondition(!!GetObjectsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBObject::GetTableName() + " manager not configured");

                TVector<TDBObject> objects;
                ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects container");

                auto session = GetServer().GetPropositionsManager()->BuildNativeSession(false);
                for (auto&& i : objects) {
                    NPropositions::TDBProposition proposition(new NPropositions::TDBEntityUpsert<TDBObject>(i));
                    ReqCheckCondition(proposition.CheckPolicy(GetServer()), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, NPropositions::TDBProposition::GetTableName() + " wrong proposition policy configuration");
                    TBase::template ReqCheckPermissions<TPropositionPermissions>(permissions, TPropositionPermissions::EObjectAction::Add, proposition.GetProposedObject());
                    if (!GetServer().GetPropositionsManager()->UpsertObject(proposition, permissions->GetUserId(), session, nullptr, nullptr)) {
                        session.DoExceptionOnFail(ConfigHttpStatus);
                    }
                }
                if (!session.Commit()) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsRemove: public TCommonRemove<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonRemove<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDBEntitiesManager<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return TDBObject::GetTypeName() + "-remove";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                ReqCheckCondition(!!GetObjectsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBObject::GetTableName() + " manager not configured");

                TSet<typename TDBObject::TId> objectIds;
                ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "objects", objectIds), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "cannot deserialize objects as ids");

                auto session = GetObjectsManager()->BuildNativeSession(false);
                TVector<TDBObject> objects;
                ReqCheckCondition(GetObjectsManager()->DirectRestoreObjects(objectIds, objects, session), ConfigHttpStatus.UnknownErrorStatus, "cannot restore objects for test");
                for (auto&& i : objects) {
                    TBase::template ReqCheckPermissions<TDBObjectPermissions<TDBObject>>(permissions, TObjectOperator::GetPublicObjectId(i), TDBObjectPermissions<TDBObject>::EObjectAction::Remove);
                }
                if (!GetObjectsManager()->RemoveObject(objectIds, permissions->GetUserId(), session) || !session.Commit()) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsProposeRemove: public TCommonRemove<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonRemove<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDBEntitiesManager<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;
            using TBase::GetServer;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return TDBObject::GetTypeName() + "-propose-remove";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "propositions manager not configured");
                ReqCheckCondition(!!GetObjectsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, TDBObject::GetTableName() + " manager not configured");

                TVector<TDBObject> objects;
                TSet<typename TDBObject::TId> objectIds;
                ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "objects", objectIds), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "cannot_read_ids");
                ReqCheckCondition(GetObjectsManager()->GetCustomEntities(objects, objectIds), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "cannot_fetch_ids");
                ReqCheckCondition(objects.size() == objectIds.size(), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, "incorrect_ids");

                auto session = GetServer().GetPropositionsManager()->BuildNativeSession(false);
                for (auto&& i : objects) {
                    NPropositions::TDBProposition proposition(new NPropositions::TDBEntityRemove<TDBObject>(i));
                    ReqCheckCondition(proposition.CheckPolicy(GetServer()), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, NPropositions::TDBProposition::GetTableName() + " wrong proposition policy configuration");
                    TBase::template ReqCheckPermissions<TPropositionPermissions>(permissions, TPropositionPermissions::EObjectAction::Add, proposition.GetProposedObject());
                    if (!GetServer().GetPropositionsManager()->UpsertObject(proposition, permissions->GetUserId(), session, nullptr, nullptr)) {
                        session.DoExceptionOnFail(ConfigHttpStatus);
                    }
                }
                if (!session.Commit()) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsHistoryInfoImpl: public TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDirectObjectsOperator<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;

            using THistoryEventsFilter = THistoryEventsFilterImpl<THistoryObjectDescription<TObjectEvent<TDBObject>>, TObjectEvent<TDBObject>>;

            virtual bool CheckObjectVisibility(TAtomicSharedPtr<IUserPermissions> permissions, const TDBObject& /*object*/) const = 0;

            virtual void RestoreObjectLinks(NJson::TJsonValue& /*json*/, const TDBObject& /*object*/) const {
            }
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override {
                const IDirectObjectsOperator<TDBObject>* manager = GetObjectsManager();

                auto reader = manager->template BuildHistorySequentialReader<THistoryEventsFilter>();
                ReqCheckCondition(reader.DeserializeFromJson(TBase::GetJsonData()), TBase::ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_objects_filter");
                {
                    auto session = manager->BuildNativeSession(true);
                    ReqCheckCondition(reader.Read(*session.GetTransaction()),
                                      ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError,
                                      "objects history restore has failed");
                }
                NJson::TJsonValue jsonHistory = NJson::JSON_ARRAY;
                ui32 hiddenObjects = 0;
                for (auto&& i : reader.GetObjects()) {
                    if (CheckObjectVisibility(permissions, i)) {
                        NJson::TJsonValue& object = jsonHistory.AppendValue(i.SerializeToJson());
                        RestoreObjectLinks(object, i);
                    } else {
                        ++hiddenObjects;
                    }
                }
                g.AddReportElement("objects", std::move(jsonHistory));
                if (reader.GetHasMore()) {
                    g.AddReportElement("cursor", reader.SerializeCursorToString());
                }
                g.AddReportElement("hidden_objects", hiddenObjects);
                g.AddReportElement("has_more", reader.GetHasMore());

            }
        };

        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsHistoryInfo: public TDBObjectsHistoryInfoImpl<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TDBObjectsHistoryInfoImpl<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual bool CheckObjectVisibility(TAtomicSharedPtr<IUserPermissions> permissions, const TDBObject& object) const override {
                return permissions->Check<TDBObjectPermissions<TDBObject>>(TObjectOperator::GetPublicObjectId(object), TDBObjectPermissions<TDBObject>::EObjectAction::HistoryObserve);
            }
        public:
            using TBase::TBase;
        };


        template <class TProduct, class TDBObject, class TUserPermissions, class TServer>
        class TDBObjectsRestoreFromHistory: public TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer> {
        private:
            using TBase = TCommonInfo<TProduct, TDBObject, TUserPermissions, TServer>;
        protected:
            virtual const IDirectObjectsOperator<TDBObject>* GetObjectsManager() const = 0;
            using TBase::ConfigHttpStatus;
            using TBase::GetJsonData;
            using TBase::ReqCheckCondition;

            virtual void RestoreObjectLinks(const NJson::TJsonValue& /*jsonLinks*/, const TDBObject& /*object*/, const TString& /*userId*/, NCS::TEntitySession& /*session*/) const {
            }

            virtual void RemoveLinks(const typename TDBObject::TId& /*objectId*/, const TString& /*userId*/, NCS::TEntitySession& /*session*/) {;
            }

            virtual void CheckPermissions(const TDBObject& dbObject, TSystemUserPermissions::TPtr permissions) {
                TBase::template ReqCheckPermissions<TDBObjectPermissions<TDBObject>>(permissions, TObjectOperator::GetPublicObjectId(dbObject), TDBObjectPermissions<TDBObject>::EObjectAction::HistoryObserve);
                TBase::template ReqCheckPermissions<TDBObjectPermissions<TDBObject>>(permissions, TObjectOperator::GetPublicObjectId(dbObject), TDBObjectPermissions<TDBObject>::EObjectAction::Modify);
            }
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override {
                Y_UNUSED(g);
                const IDirectObjectsOperator<TDBObject>* manager = GetObjectsManager();
                ReqCheckCondition(manager, TBase::ConfigHttpStatus.NoContentStatus, "manager is not configured");
                ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
                for (auto&& jsonObject : GetJsonData()["objects"].GetArraySafe()) {
                    typename TDBObject::TId internalId;
                    ReqCheckCondition(TJsonProcessor::Read(jsonObject, TDBObject::GetIdFieldName(), internalId, true), TBase::ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_internal_id");
                    ui64 historyEventId;
                    ReqCheckCondition(TJsonProcessor::Read(jsonObject, "history_event_id", historyEventId, true), TBase::ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_history_event_id");
                    auto session = manager->BuildNativeSession(false);
                    RemoveLinks(internalId, permissions->GetUserId(), session);

                    {
                        TMaybe<TObjectEvent<TDBObject>> result;
                        ReqCheckCondition(manager->RestoreHistoryEvent(historyEventId, result, session),
                            TBase::ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "restore history event has failed");
                        ReqCheckCondition(!!result, TBase::ConfigHttpStatus.UserErrorState, "incorrect event_id. cannot restore object");
                        CheckPermissions(*result, permissions);
                    }

                    {
                        TMaybe<TDBObject> result;
                        ReqCheckCondition(manager->RestoreHistoryEventAsCurrent(historyEventId, result, permissions->GetUserId(), session),
                            TBase::ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "restore history event has failed");
                        ReqCheckCondition(!!result, TBase::ConfigHttpStatus.NoContentStatus, "no such event for given history_event_id: " + ::ToString(historyEventId));
                        ReqCheckCondition(::ToString(internalId) == ToString(result->GetInternalId()), TBase::ConfigHttpStatus.SyntaxErrorStatus, "provided internalId didn't match with the restored from history; provided_internal_id: "
                            + ::ToString(internalId) + " restored_internal_id: " + ::ToString(result->GetInternalId()));
                        RestoreObjectLinks(jsonObject["links"], *result, permissions->GetUserId(), session);
                    }
                    ReqCheckCondition(session.Commit(), TBase::ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "session commit has failed");
                }
            }
        };
    }
}
