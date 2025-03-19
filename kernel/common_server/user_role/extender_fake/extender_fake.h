#pragma once
#include <kernel/common_server/user_role/abstract/abstract.h>

namespace NCS {

    class TUserInfoExtenderFakeConfig final : public IPermissionsManagerConfig::IUserInfoExtenderConfig {
        static TFactory::TRegistrator<TUserInfoExtenderFakeConfig> Registrator;
    protected:
        virtual void DoToString(IOutputStream& os) const override;
        virtual void DoInit(const TYandexConfig::Section* section) override;

    public:
        static TString GetTypeName();
        virtual TString GetClassName() const override;
        virtual TAtomicSharedPtr<IPermissionsManager::IUserInfoExtender> ConstructUserInfoExtender(const IBaseServer& server) const override;
    };

    class TUserInfoExtenderFake final: public IPermissionsManager::IUserInfoExtender {
    public:
        virtual bool FillExtendedInfo(TExtendedUserInfo& result, const IPermissionsManager::TGetPermissionsContext& context) const override;
    };

}
