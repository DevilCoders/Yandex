#include "filter.h"
#include <kernel/common_server/processors/tags/object.h>
#include <kernel/common_server/processors/tags/direct.h>

namespace NCS {
    namespace NTags {
        void TFilterByTagNames::FillFilter(TSRMulti& srMulti, NCS::NSelection::IExternalData::TPtr externalData) const {
            auto ted = DynamicPointerCast<TTagsSelectionExternalData>(externalData);
            if (!ted) {
                return;
            }
            IUserPermissions::TPtr userPermissions = ted->GetUserPermissions();
            if (!userPermissions) {
                return;
            }
            const TString userId = userPermissions ? userPermissions->GetUserId() : "";
            TSet<TString> tagNames = userPermissions->GetObjectNames<NHandlers::TTagPermissions>(NHandlers::ETagAction::Observe);
            if (tagNames.contains("*")) {
                if (TagNames) {
                    tagNames = *TagNames;
                } else {
                    return;
                }
            } else if (TagNames) {
                for (auto it = tagNames.begin(); it != tagNames.end();) {
                    if (!TagNames->contains(*it)) {
                        it = tagNames.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            if (tagNames.size() || userId) {
                auto& mAffectedCase = srMulti.RetNode<TSRMulti>(ESRMulti::Or);
                if (tagNames.size()) {
                    mAffectedCase.InitNode<TSRBinary>("tag_name", tagNames);
                }
                if (userId) {
                    mAffectedCase.InitNode<TSRBinary>("performer_id", userId);
                }
            }
        }

        void TObjectsFilterByTagNames::FillFilter(TSRMulti& srMulti, NSelection::IExternalData::TPtr externalData) const {
            auto ted = DynamicPointerCast<TTagsSelectionExternalData>(externalData);
            if (!ted) {
                return;
            }
            IUserPermissions::TPtr userPermissions = ted->GetUserPermissions();
            if (!userPermissions) {
                return;
            }
            const TString userId = userPermissions ? userPermissions->GetUserId() : "";
            TSet<TString> tagNames = userPermissions->GetObjectNames<NHandlers::TTagsObjectPermissions>(TAdministrativePermissions::EObjectAction::Observe);
            if (tagNames.contains("*")) {
                return;
            }
            if (TagNames) {
                for (auto it = tagNames.begin(); it != tagNames.end();) {
                    if (!TagNames->contains(*it)) {
                        it = tagNames.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            if (tagNames.size() || userId) {
                auto& mAffectedCase = srMulti.RetNode<TSRMulti>(ESRMulti::Or);
                if (tagNames.size()) {
                    mAffectedCase.InitNode<TSRBinary>("tag_name", tagNames);
                }
                if (userId) {
                    mAffectedCase.InitNode<TSRBinary>("performer_id", userId);
                }
            }
        }

    }
}
