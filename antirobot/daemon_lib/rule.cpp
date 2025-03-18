#include "rule.h"


namespace NAntiRobot {


bool TRule::Empty() const {
    return
        !IsTor &&
        !IsProxy &&
        !IsVpn &&
        !IsHosting &&
        !IsMobile &&
        !IsMikrotik &&
        !IsSquid &&
        !IsDdoser &&
        Doc.empty() &&
        CgiString.empty() &&
        IdentType.empty() &&
        ArrivalTime.empty() &&
        ServiceType.empty() &&
        Request.empty() &&
        Hodor.empty() &&
        HodorHash.empty() &&
        JwsInfo.empty() &&
        YandexTrustInfo.empty() &&
        CountryId.empty() &&
        Headers.empty() &&
        HasHeaders.empty() &&
        NumHeaders.empty() &&
        CsHeaders.empty() &&
        HasCsHeaders.empty() &&
        NumCsHeaders.empty() &&
        IpInterval.IsEmpty() &&
        !CbbGroup.Defined() &&
        !IsWhitelist.Defined() &&
        !Degradation.Defined() &&
        !PanicMode.Defined() &&
        !InRobotSet.Defined() &&
        !MayBan.Defined() &&
        !RandomThreshold.Defined() &&
        !ExpBin.Defined() &&
        !ValidAutoRuTamper.Defined() &&
        CookieAge.IsNop() &&
        CurrentTimestamp.empty() &&
        Factors.empty();
}


} // namespace NAntiRobot
