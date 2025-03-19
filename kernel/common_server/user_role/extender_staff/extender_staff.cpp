#include "extender_staff.h"
#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {

    TUserInfoExtenderStaffConfig::TFactory::TRegistrator<TUserInfoExtenderStaffConfig> TUserInfoExtenderStaffConfig::Registrator(TUserInfoExtenderStaffConfig::GetTypeName());

    void TUserInfoExtenderStaffConfig::DoToString(IOutputStream& os) const {
        os << "StaffApi: " << StaffApi << Endl;
        os << "AuthModuleNames: " << JoinSeq(", ", AuthModuleNames) << Endl;
    }

    void TUserInfoExtenderStaffConfig::DoInit(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        AssertCorrectConfig(dir.GetNonEmptyValue("StaffApi", StaffApi), "StaffApi not set");
        StringSplitter(dir.Value<TString>("AuthModuleNames")).SplitBySet(", ").SkipEmpty().ParseInto(&AuthModuleNames);
    }

    TString TUserInfoExtenderStaffConfig::GetTypeName() {
        return "staff";
    }

    TString TUserInfoExtenderStaffConfig::GetClassName() const {
        return GetTypeName();
    }

    TAtomicSharedPtr<IPermissionsManager::IUserInfoExtender> TUserInfoExtenderStaffConfig::ConstructUserInfoExtender(const IBaseServer& server) const {
        auto sender = server.GetSenderPtr(StaffApi);
        AssertCorrectConfig(!!sender, "Unknown StaffApi");
        return MakeAtomicShared<TUserInfoExtenderStaff>(*this, sender);
    }

    TUserInfoExtenderStaff::TUserInfoExtenderStaff(const TUserInfoExtenderStaffConfig& config, NExternalAPI::TSender::TPtr staffSender)
        : Config(config)
        , StaffClient(staffSender)
    {}

    bool TUserInfoExtenderStaff::FillExtendedInfo(TExtendedUserInfo& result, const IPermissionsManager::TGetPermissionsContext& context) const {
        result.UserIdAliases.emplace(context.GetUserId());
        if (Config.GetAuthModuleNames().empty() || Config.GetAuthModuleNames().contains(context.GetAuthModuleName())) {
            TStaffClient::TStaffEntries enries;
            if (!StaffClient.GetUserData(TStaffEntrySelector(TStaffEntrySelector::EStaffEntryField::Uid, context.GetUserId()), enries, { TStaffEntry::EStaffEntryField::GroupId }, 1)) {
                return false;
            }
            for (const auto& e : enries) {
                for (const auto& g : e.GetGroups()) {
                    result.UserIdAliases.emplace("staff_group:" + ToString(g.GetId()));
                }
            }
        }
        return true;
    }
}
