#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/processors/db_entity/handler.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/tags/manager.h>
#include <kernel/common_server/tags/objects_filter.h>

namespace NCS {
    namespace NHandlers {
        using TTagActions = ui64;
        enum class ETagAction: TTagActions {
            Add = 1 /* "add" */,
            Modify = 1 << 1 /* "modify" */,
            Observe = 1 << 2 /* "observe" */,
            Remove = 1 << 3 /* "remove" */,
            HistoryObserve = 1 << 4 /* "history_observe" */,
            TakePerform = 1 << 5 /* "take_perform" */,
            DropPerform = 1 << 6 /* "drop_perform" */
        };

        class TTagPermissions: public TAdministrativePermissionsImpl<ETagAction, TTagActions> {
        public:
            enum class EPerformingPolicy {
                None,
                OnlyMine,
                OnlyMineOrEmpty
            };
        private:
            using TBase = TAdministrativePermissionsImpl<ETagAction, TTagActions>;
            static TFactory::TRegistrator<TTagPermissions> Registrator;

            CS_ACCESS(TTagPermissions, EPerformingPolicy, PerformingPolicy, EPerformingPolicy::OnlyMineOrEmpty);
            CSA_DEFAULT(TTagPermissions, TSet<TString>, Tags);
            CSA_MAYBE(TTagPermissions, NCS::NTags::TObjectFilter, ObjectTagsFilter);
        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
        public:
            const TSet<TString>& GetObjectNames() const {
                return Tags;
            }

            static TString GetTypeName() {
                return "tags";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            template <class TExternalTag>
            bool Check(const EObjectAction& action, const TExternalTag& t) const {
                if (!GetActions().contains(action)) {
                    return false;
                }
                if (!Tags.contains("*") && !Tags.contains(t.GetName())) {
                    return false;
                }
                return true;
            }

            template <class TExternalTag>
            bool Check(const TString& userId, const EObjectAction action, const TVector<TExternalTag>& tags, const TExternalTag& t) const {
                switch (PerformingPolicy) {
                    case EPerformingPolicy::None:
                        break;
                    case EPerformingPolicy::OnlyMine:
                        if (t.GetPerformerId() != userId) {
                            return false;
                        }
                        break;
                    case EPerformingPolicy::OnlyMineOrEmpty:
                        if (t.GetPerformerId() != userId && !!t.GetPerformerId()) {
                            return false;
                        }
                        break;
                }
                if (!Check(action, t)) {
                    return false;
                }
                if (ObjectTagsFilter && !ObjectTagsFilter->Check(tags)) {
                    return false;
                }
                return true;
            }

            virtual NJson::TJsonValue SerializeToJson() const override;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override;
        };

        template <class TDBExternalTag, class TProductClass>
        class TTagsListProcessor: public TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>> {
            using TBase = TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>>;
            using TBase::Context;
        protected:
            virtual const IDirectTagsManager<TDBExternalTag>& GetTagsManager() const = 0;
            virtual NJson::TJsonValue GetJsonReport(TAtomicSharedPtr<IUserPermissions> /*permissions*/, const TDBExternalTag& dbTag) const {
                return dbTag.SerializeToJson();
            }
            virtual bool DoFillHandlerScheme(NScheme::THandlerScheme& scheme, const IBaseServer& server) const override {
                auto& rMethod = scheme.Method(NScheme::ERequestMethod::Post);
                rMethod.MutableParameters().AddQuery<TFSString>("ids").SetRequired(false);
                auto& content = rMethod.Body().Content();
                content.Add<TFSString>("object_id").SetRequired(false);
                content.Add<TFSArray>("object_ids").SetElement<TFSString>().SetRequired(false);
                content.Add<TFSStructure>("selection").SetStructure(TDBExternalTag::GetSearchScheme(server));
                auto& replyScheme = rMethod.Response(HTTP_OK).AddContent();
                replyScheme.Add<TFSArray>("objects").SetElement(TDBExternalTag::GetScheme(server));
                return true;
            }
        public:
            using TBase::TBase;
            using TBase::GetJsonData;
            using TBase::ConfigHttpStatus;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TAtomicSharedPtr<TSystemUserPermissions> permissions) override {
                const IDirectTagsManager<TDBExternalTag>& manager = GetTagsManager();
                const bool isPlain = IsTrue(TBase::GetCgiParameters().Get("plain_report"));
                TSet<TString> idsStr = MakeSet(TBase::GetStrings(Context->GetCgiParameters(), "ids", false));
                TSet<TString> names;
                const NJson::TJsonValue& postData = GetJsonData();
                TVector<TTaggedObject<TDBExternalTag>> objects;
                bool isSelection = false;
                if (postData.Has("object_id")) {
                    idsStr = { postData["object_id"].GetStringRobust() };
                } else if (postData.Has("object_ids") && postData["object_ids"].IsArray()) {
                    for (auto&& i : postData["object_ids"].GetArraySafe()) {
                        idsStr.emplace(i.GetStringRobust());
                    }
                } else if (postData.Has("filter")) {
                    const auto& filter = postData["filter"];
                    if (filter.Has("names")) {
                        const auto& namesJson = filter["names"];
                        TBase::ReqCheckCondition(namesJson.IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'names' is not array");
                        for (auto&& i : namesJson.GetArraySafe()) {
                            TBase::ReqCheckCondition(i.IsString(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect names list");
                            names.emplace(i.GetString());
                        }
                    }
                } else if (postData.Has("selection")) {
                    auto reader = manager.BuildSequentialReader();
                    TBase::ReqCheckCondition(reader.DeserializeFromJson(postData["selection"]), ConfigHttpStatus.SyntaxErrorStatus, "cannot parse selection");
                    auto session = manager.BuildNativeSession(true);
                    TBase::ReqCheckCondition(reader.Read(*session.GetTransaction(), new NCS::NTags::TTagsSelectionExternalData(permissions)), ConfigHttpStatus.UnknownErrorStatus, "cannot read data");
                    objects = manager.BuildTaggedObjectsSorted(reader.DetachObjects());
                    isSelection = true;
                    g.MutableReport().AddReportElement("cursor", reader.SerializeCursorToString());
                    g.MutableReport().AddReportElement("has_more", reader.GetHasMore());
                }
                if (!isSelection) {
                    TSet<typename TDBExternalTag::TObjectId> ids;
                    for (auto&& i : idsStr) {
                        typename TDBExternalTag::TObjectId objectId;
                        TBase::ReqCheckCondition(TryFromString(i, objectId), ConfigHttpStatus.SyntaxErrorStatus, "incorrect object id type");
                        ids.emplace(objectId);
                    }
                    {
                        auto session = manager.BuildNativeSession(true);
                        if (!manager.RestoreSortedObjectsById(ids, objects, session)) {
                            session.DoExceptionOnFail(ConfigHttpStatus);
                        }
                    }
                }
                NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
                for (auto&& i : objects) {
                    if (isPlain) {
                        for (auto&& j : i.GetTags()) {
                            if (permissions->Check<TTagPermissions>(permissions->GetUserId(), TTagPermissions::EObjectAction::Observe, i.GetTags(), j)) {
                                objectsJson.AppendValue(j.SerializeToJson());
                            }
                        }
                    } else {
                        auto& objectJson = objectsJson.AppendValue(NJson::JSON_MAP);
                        objectJson.InsertValue("object_id", i.GetObjectId());
                        auto& tagsJson = objectJson.InsertValue("tags", NJson::JSON_ARRAY);
                        for (auto&& j : i.GetTags()) {
                            if (permissions->Check<TTagPermissions>(permissions->GetUserId(), TTagPermissions::EObjectAction::Observe, i.GetTags(), j)) {
                                tagsJson.AppendValue(j.SerializeToJson());
                            }
                        }
                    }
                }
                g.MutableReport().AddReportElement("objects", std::move(objectsJson));

                const TCgiParameters& cgiParameters = Context->GetCgiParameters();
                NJson::TJsonValue reportMetaJson;
                if (TString reportMeta = cgiParameters.Get("report_meta"); NJson::ReadJsonTree(reportMeta, &reportMetaJson)) {
                    g.MutableReport().AddReportElement("report_meta", std::move(reportMetaJson));
                }
            }
        };

        template <class TDBExternalTag, class TProductClass>
        class TTagsUpsertProcessor: public TCommonUpsert<TProductClass, TDBExternalTag, TSystemUserPermissions, IBaseServer> {
            using TBase = TCommonUpsert<TProductClass, TDBExternalTag, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual const IDirectTagsManager<TDBExternalTag>& GetTagsManager() const = 0;
            using TBase::ReqCheckCondition;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckPermissions;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                const IDirectTagsManager<TDBExternalTag>& manager = GetTagsManager();
                TVector<TDBExternalTag> tags;
                TSet<typename TDBExternalTag::TObjectId> tagObjectIds;
                {
                    const NJson::TJsonValue::TArray* arrTags = nullptr;
                    if (TBase::GetJsonData().IsArray()) {
                        ReqCheckCondition(TBase::GetJsonData().GetArrayPointer(&arrTags), TBase::ConfigHttpStatus.SyntaxErrorStatus, "tags_is_not_array");
                    } else if (TBase::GetJsonData().Has("objects")) {
                        ReqCheckCondition(TBase::GetJsonData()["objects"].GetArrayPointer(&arrTags), TBase::ConfigHttpStatus.SyntaxErrorStatus, "tags.objects_is_not_array");
                    } else {
                        ReqCheckCondition(false, TBase::ConfigHttpStatus.SyntaxErrorStatus, "incorrect_request_objects");
                    }

                    for (auto&& i : *arrTags) {
                        ReqCheckCondition(i.IsMap(), TBase::ConfigHttpStatus.SyntaxErrorStatus, "tag_container_not_map");
                        ReqCheckCondition(i["tag_name"].IsString(), TBase::ConfigHttpStatus.SyntaxErrorStatus, "tag_name is not string");
                        const TString& tagName = i["tag_name"].GetString();
                        auto td = TBase::GetServer().GetTagDescriptionsManager().GetCustomObject(tagName);
                        ReqCheckCondition(!!td, TBase::ConfigHttpStatus.UserErrorState, "tag_name_is_incorrect: " + tagName);

                        TDBExternalTag container;
                        ReqCheckCondition(container.DeserializeFromJson(i, td->GetClassName()), ConfigHttpStatus.UserErrorState, "cannot parse tag: " + tagName);
                        tagObjectIds.emplace(container.GetObjectId());
                        tags.emplace_back(std::move(container));
                    }
                }

                auto session = manager.BuildNativeSession(false);
                TMap<typename TDBExternalTag::TObjectId, TTaggedObject<TDBExternalTag>> taggedObjects;
                if (!manager.RestoreSortedObjectsById(tagObjectIds, taggedObjects, session)) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }

                for (auto&& i : tags) {
                    bool isUpdate = false;
                    TDBExternalTag t;
                    auto td = TBase::GetServer().GetTagDescriptionsManager().GetCustomObject(i.GetName());
                    ReqCheckCondition(!!td, TBase::ConfigHttpStatus.UserErrorState, "tag_name_is_incorrect: " + i.GetName());
                    if (!manager.SmartUpsert(i, *td, permissions->GetUserId(), session, &isUpdate, &t)) {
                        session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                    }
                    auto it = taggedObjects.find(i.GetObjectId());
                    TVector<TDBExternalTag> objectTags;
                    if (it != taggedObjects.end()) {
                        objectTags = it->second.GetTags();
                    }
                    if (isUpdate) {
                        TBase::template ReqCheckPermissions<TTagPermissions>(permissions, permissions->GetUserId(), ETagAction::Modify, objectTags, t);
                    } else {
                        TBase::template ReqCheckPermissions<TTagPermissions>(permissions, permissions->GetUserId(), ETagAction::Add, objectTags, t);
                    }

                }
                if (!session.Commit()) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
            }
        };

        template <class TDBExternalTag, class TProductClass>
        class TTagsRemoveProcessor: public TCommonRemove<TProductClass, TDBExternalTag, TSystemUserPermissions, IBaseServer> {
            using TBase = TCommonRemove<TProductClass, TDBExternalTag, TSystemUserPermissions, IBaseServer>;
        protected:
            virtual const IDirectTagsManager<TDBExternalTag>& GetTagsManager() const = 0;
            using TBase::ReqCheckCondition;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckPermissions;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                const IDirectTagsManager<TDBExternalTag>& manager = GetTagsManager();
                TSet<typename TDBExternalTag::TTagId> idsForRemove;
                auto session = manager.BuildNativeSession(false);
                {
                    TSet<typename TDBExternalTag::TTagId> ids;
                    ReqCheckCondition(TJsonProcessor::ReadContainer(TBase::GetJsonData(), "objects", ids), TBase::ConfigHttpStatus.SyntaxErrorStatus, "objects_field_is_incorrect");
                    TVector<TTaggedObject<TDBExternalTag>> taggedObjects;
                    {
                        if (!manager.RestoreObjectsByTagIds(ids, taggedObjects, true, session)) {
                            session.DoExceptionOnFail(ConfigHttpStatus);
                        }
                    }
                    for (auto&& tObject : taggedObjects) {
                        for (auto&& t : tObject.GetTags()) {
                            if (!ids.contains(t.GetTagId())) {
                                continue;
                            }
                            TBase::template ReqCheckPermissions<TTagPermissions>(permissions, permissions->GetUserId(), ETagAction::Remove, tObject.GetTags(), t);
                            idsForRemove.emplace(t.GetTagId());
                        }
                    }
                }

                if (!manager.RemoveTags(idsForRemove, permissions->GetUserId(), session)) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
                if (!session.Commit()) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
            }
        };

        template <class TDBExternalTag, class TProductClass>
        class TTagsHistoryProcessor: public TDBObjectsHistoryInfoImpl<TProductClass, TDBExternalTag, TSystemUserPermissions, IBaseServer> {
            using TBase = TDBObjectsHistoryInfoImpl<TProductClass, TDBExternalTag, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;
            using typename TBase::THistoryEventsFilter;

            virtual bool CheckObjectVisibility(TAtomicSharedPtr<IUserPermissions> permissions, const TDBExternalTag& object) const {
                if (!object) {
                    return false;
                }
                return permissions->Check<TTagPermissions>(ETagAction::HistoryObserve, object);
            }
        };
    }
}
