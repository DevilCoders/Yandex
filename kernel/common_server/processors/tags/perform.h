#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/tags/manager.h>
#include <kernel/common_server/tags/objects_filter.h>
#include "direct.h"

namespace NCS {
    namespace NHandlers {
        template <class TDBExternalTag, class TProductClass>
        class TSetTagsPerformerProcessor: public TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>> {
            using TBase = TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>>;
        protected:
            virtual const IDirectTagsManager<TDBExternalTag>& GetTagsManager() const = 0;
            using TBase::ReqCheckCondition;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckPermissions;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                const IDirectTagsManager<TDBExternalTag>& manager = GetTagsManager();
                const TSet<typename TDBExternalTag::TTagId> ids = MakeSet(TBase::template GetValues<typename TDBExternalTag::TTagId>(TBase::GetJsonData(), "objects", false));
                auto session = manager.BuildNativeSession(false, true);
                TVector<TTaggedObject<TDBExternalTag>> objects;
                ReqCheckCondition(manager.RestoreObjectsByTagIds(ids, objects, true, session), ConfigHttpStatus.UnknownErrorStatus, "cannot_restore_objects_by_tag_ids");

                for (auto&& i : objects) {
                    for (auto&& t : i.GetTags()) {
                        if (!ids.contains(t.GetTagId())) {
                            continue;
                        }
                        TBase::template ReqCheckPermissions<TTagPermissions>(permissions, permissions->GetUserId(), TTagPermissions::EObjectAction::TakePerform, i.GetTags(), t);
                    }
                }

                TVector<TDBExternalTag> tags;
                if (!manager.SetTagsPerformer(ids, permissions->GetUserId(), session, &tags)) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
                ReqCheckCondition(tags.size() == ids.size(), ConfigHttpStatus.UnknownErrorStatus, "cannot_set_performer_for_all_tag_ids");

                if (!session.Commit()) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
            }
        };

        template <class TDBExternalTag, class TProductClass>
        class TDropTagsPerformerProcessor: public TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>> {
            using TBase = TRequestWithRegistrator<TProductClass, TRequestWithPermissions<TSystemUserPermissions, IBaseServer>>;
        protected:
            virtual const IDirectTagsManager<TDBExternalTag>& GetTagsManager() const = 0;
            using TBase::ReqCheckCondition;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckPermissions;
        public:
            using TBase::TBase;

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                const IDirectTagsManager<TDBExternalTag>& manager = GetTagsManager();
                const TSet<typename TDBExternalTag::TTagId> ids = MakeSet(TBase::template GetValues<typename TDBExternalTag::TTagId>(TBase::GetJsonData(), "objects", false));
                auto session = manager.BuildNativeSession(false, true);
                TVector<TTaggedObject<TDBExternalTag>> objects;
                ReqCheckCondition(manager.RestoreObjectsByTagIds(ids, objects, true, session), ConfigHttpStatus.UnknownErrorStatus, "cannot_restore_objects_by_tag_ids");

                for (auto&& i : objects) {
                    for (auto&& t : i.GetTags()) {
                        if (!ids.contains(t.GetTagId())) {
                            continue;
                        }
                        TBase::template ReqCheckPermissions<TTagPermissions>(permissions, permissions->GetUserId(), TTagPermissions::EObjectAction::DropPerform, i.GetTags(), t);
                    }
                }

                TVector<TDBExternalTag> tags;
                if (!manager.DropTagsPerformer(ids, permissions->GetUserId(), session, &tags)) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
                ReqCheckCondition(tags.size() == ids.size(), ConfigHttpStatus.UnknownErrorStatus, "cannot_drop_performer_for_all_tag_ids");

                if (!session.Commit()) {
                    session.DoExceptionOnFail(TBase::ConfigHttpStatus);
                }
            }
        };

    }
}
