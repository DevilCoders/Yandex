#pragma once
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/roles/db_roles/role.h>
#include <kernel/common_server/roles/db_roles/manager.h>
#include <kernel/common_server/proposition/actions/abstract.h>

namespace NCS {
    namespace NPropositions {

        class TRoleAction: public IProposedActionWithDelay {
        private:
            using TBase = IProposedActionWithDelay;
            CSA_DEFAULT(TRoleAction, TUserRoleInfo, UserRole);

        protected:
            const TDBFullRolesManager* GetFullRolesManager(const IBaseServer& server) const {
                return dynamic_cast<const TDBFullRolesManager*>(&server.GetRolesManager());
            }

        public:
            TRoleAction() = default;
            TRoleAction(const TUserRoleInfo& userRole)
                : UserRole(userRole)
            {
            }

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                result.InsertValue("userRole", UserRole.SerializeToJson());
                return result;
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                if (!TJsonProcessor::ReadObject(jsonData, "userRole", UserRole)) {
                    return false;
                }
                return TBase::DeserializeFromJson(jsonData);
            }
            virtual TString GetObjectId() const override {
                return UserRole.GetRoleName();
            }
            virtual TString GetCategoryId() const override {
                return "user_role_info";
            }
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                NCS::NScheme::TScheme result = TBase::GetScheme(server);
                result.Add<TFSStructure>("userRole").SetStructure(TUserRoleInfo::GetScheme(server));
                return result;
            }
        };

        class TRoleActionRemove: public TRoleAction {
        private:
            using TBase = TRoleAction;
            static TBase::TFactory::TRegistrator<TRoleActionRemove> Registrator;

        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override {
                auto* manager = TBase::GetFullRolesManager(server);
                if (!manager) {
                    TFLEventLog::Error("incorrect manager");
                    return false;
                }
                return manager->RemoveRoles({TBase::GetObjectId()}, userId);
            }

        public:
            using TBase::TBase;
            static TString GetTypeName() {
                return "user_role__remove";
            }
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TRoleActionUpsert: public TRoleAction {
        private:
            using TBase = TRoleAction;
            static TBase::TFactory::TRegistrator<TRoleActionUpsert> Registrator;

        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override {
                auto* manager = TBase::GetFullRolesManager(server);
                if (!manager) {
                    TFLEventLog::Error("incorrect manager");
                    return false;
                }
                return manager->UpsertRole(TBase::GetUserRole(), userId);
            }

        public:
            using TBase::TBase;
            static TString GetTypeName() {
                return "user_role__upsert";
            }
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
