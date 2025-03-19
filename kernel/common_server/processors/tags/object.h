#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/tags/manager.h>
#include <kernel/common_server/tags/filter.h>
#include <kernel/common_server/tags/objects_filter.h>

namespace NCS {
    namespace NHandlers {

        class TTagsObjectPermissions: public TAdministrativePermissions {
        private:
            using TBase = TAdministrativePermissions;
            static TFactory::TRegistrator<TTagsObjectPermissions> Registrator;

            CSA_DEFAULT(TTagsObjectPermissions, NCS::NTags::TObjectFilter, Filter);
        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
                NFrontend::TScheme result = TBase::DoGetScheme(server);
                result.Add<TFSString>("filter");
                return result;
            }
        public:
            template <class TDBObjectTag>
            bool Check(const TTaggedObject<TDBObjectTag>& taggedObject, const TAdministrativePermissions::EObjectAction action) const {
                if (!GetActions().contains(action)) {
                    return false;
                }
                return Filter.Check(taggedObject);
            }

            TSet<TString> GetObjectNames() const {
                return Filter.GetAffectedTagNames();
            }

            static TString GetTypeName() {
                return "tags_object";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                TJsonProcessor::Write(result, "filter", Filter.SerializeToString());
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override {
                if (!TBase::DeserializeFromJson(info)) {
                    return false;
                }
                TString tagsFilter;
                if (!TJsonProcessor::Read(info, "filter", tagsFilter, true)) {
                    return false;
                }
                if (!Filter.DeserializeFromString(tagsFilter)) {
                    return false;
                }
                return true;
            }
        };

        template <class TDBExternalTag, class TProductClass>
        class TTaggedObjectsInfoHandler: public TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>> {
            using TBase = TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>>;
            using TBase::Context;
        protected:
            using TBase::ConfigHttpStatus;
            virtual const IDirectTagsManager<TDBExternalTag>& GetTagsManager() const = 0;
            virtual void BuildReportFinally(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions, const TMap<typename TDBExternalTag::TObjectId, const TTaggedObject<TDBExternalTag>*>& taggedObjectsPtr) const = 0;
        public:
            using TBase::TBase;
            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override {
                auto reader = GetTagsManager().BuildObjectsSequentialReader();
                reader.SetNoDataOnEmptyFilter(false);
                reader.SetCountLimit(TBase::GetServer().GetSettings().template GetValueDef<ui32>("limits.tags_for_objects_detection", 1000000));
                TBase::ReqCheckCondition(reader.DeserializeFromJson(TBase::GetJsonData()), ConfigHttpStatus.SyntaxErrorStatus, "cannot_parse_filter");
                {
                    auto session = GetTagsManager().BuildNativeSession(true);
                    TBase::ReqCheckCondition(reader.Read(*session.GetTransaction(), new NTags::TTagsSelectionExternalData(permissions)), ConfigHttpStatus.UnknownErrorStatus, "cannot_read_data");
                }
                if (reader.GetHasMore()) {
                    g.AddReportElement("cursor", reader.SerializeCursorToString());
                }
                g.AddReportElement("has_more", reader.GetHasMore());
                auto tObjects = GetTagsManager().BuildTaggedObjectsSorted(reader.DetachObjects());
                ui32 hidden = 0;
                TMap<typename TDBExternalTag::TObjectId, const TTaggedObject<TDBExternalTag>*> taggedObjectsPtr;
                for (auto&& i : tObjects) {
                    if (permissions->Check<TTagsObjectPermissions>(i, TAdministrativePermissions::EObjectAction::Observe)) {
                        taggedObjectsPtr.emplace(i.GetObjectId(), &i);
                    } else {
                        ++hidden;
                    }
                }
                g.AddReportElement("hidden", hidden);
                BuildReportFinally(g, permissions, taggedObjectsPtr);
            }
        };
    }
}
