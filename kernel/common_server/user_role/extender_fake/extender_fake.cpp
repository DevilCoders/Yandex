#include "extender_fake.h"

namespace NCS {

    TUserInfoExtenderFakeConfig::TFactory::TRegistrator<TUserInfoExtenderFakeConfig> TUserInfoExtenderFakeConfig::Registrator(TUserInfoExtenderFakeConfig::GetTypeName());

    void TUserInfoExtenderFakeConfig::DoToString(IOutputStream& /*os*/) const {
    }

    void TUserInfoExtenderFakeConfig::DoInit(const TYandexConfig::Section* /*section*/) {
    }

    TString TUserInfoExtenderFakeConfig::GetTypeName() {
        return IPermissionsManagerConfig::TUserInfoExtenderConfig::TConfigurationClass::GetDefaultClassName();
    }

    TString TUserInfoExtenderFakeConfig::GetClassName() const {
        return GetTypeName();
    }

    TAtomicSharedPtr<IPermissionsManager::IUserInfoExtender> TUserInfoExtenderFakeConfig::ConstructUserInfoExtender(const IBaseServer& /*server*/) const {
        return MakeAtomicShared<TUserInfoExtenderFake>();
    }

    bool TUserInfoExtenderFake::FillExtendedInfo(TExtendedUserInfo& result, const IPermissionsManager::TGetPermissionsContext& context) const {
        result.UserIdAliases.emplace(context.GetUserId());
        return true;
    }
}
