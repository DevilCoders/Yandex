#pragma once
#include <kernel/common_server/user_role/abstract/abstract.h>
#include <kernel/common_server/library/staff/client.h>

namespace NCS {

    class TUserInfoExtenderStaffConfig final : public IPermissionsManagerConfig::IUserInfoExtenderConfig {
        static TFactory::TRegistrator<TUserInfoExtenderStaffConfig> Registrator;
        CSA_READONLY_DEF(TString, StaffApi);
        CSA_READONLY_DEF(TSet<TString>, AuthModuleNames);
    protected:
        virtual void DoToString(IOutputStream& os) const override;
        virtual void DoInit(const TYandexConfig::Section* section) override;

    public:
        static TString GetTypeName();
        virtual TString GetClassName() const override;
        virtual TAtomicSharedPtr<IPermissionsManager::IUserInfoExtender> ConstructUserInfoExtender(const IBaseServer& server) const override;
    };

    class TUserInfoExtenderStaff : public IPermissionsManager::IUserInfoExtender {
    public:
        TUserInfoExtenderStaff(const TUserInfoExtenderStaffConfig& config, NExternalAPI::TSender::TPtr staffSender);
        virtual bool FillExtendedInfo(TExtendedUserInfo& result, const IPermissionsManager::TGetPermissionsContext& context) const override;

    private:
        const TUserInfoExtenderStaffConfig& Config;
        TStaffClient StaffClient;
    };

}
