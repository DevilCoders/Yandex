#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/tags/manager.h>

namespace NCS {
    namespace NHandlers {

        enum class ETagDescriptionActions: ui64 {
            Add = 1 /* "add" */,
            Modify = 1 << 1 /* "modify" */,
            Observe = 1 << 2 /* "observe" */,
            Remove = 1 << 3 /* "remove" */,
            ObserveDeprecated = 1 << 4 /* "observe_deprecated" */,
            RemoveDeprecated = 1 << 5 /* "remove_deprecated" */,
        };

        class TTagDescriptionPermissions: public TAdministrativePermissionsImpl<ETagDescriptionActions, ui64> {
        private:
            using TBase = TAdministrativePermissions;
            static TFactory::TRegistrator<TTagDescriptionPermissions> Registrator;

        public:
            static TString GetTypeName() {
                return "tag_descriptions";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TTagDescriptionsListProcessor: public TCommonInfo<TTagDescriptionsListProcessor, TDBTagDescription, TSystemUserPermissions, IBaseServer> {
            using TBase = TCommonInfo<TTagDescriptionsListProcessor, TDBTagDescription, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "tag_descriptions_list";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

        class TTagDescriptionsUpsertProcessor: public TCommonUpsert<TTagDescriptionsUpsertProcessor, TDBTagDescription, TSystemUserPermissions, IBaseServer> {
            using TBase = TCommonUpsert<TTagDescriptionsUpsertProcessor, TDBTagDescription, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "tag_descriptions_upsert";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

        class TTagDescriptionsRemoveProcessor: public TCommonRemove<TTagDescriptionsRemoveProcessor, TDBTagDescription, TSystemUserPermissions, IBaseServer> {
            using TBase = TCommonRemove<TTagDescriptionsRemoveProcessor, TDBTagDescription, TSystemUserPermissions, IBaseServer>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "tag_description_remove";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

    }
}
