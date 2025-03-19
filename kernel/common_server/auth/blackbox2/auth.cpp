#include "auth.h"

#include <kernel/common_server/library/json/cast.h>

NJson::TJsonValue TBlackboxAuthInfo::GetInfo() const {
    NJson::TJsonValue result;
    result.InsertValue("passport_uid", PassportUid);
    result.InsertValue("login", Login);
    result.InsertValue("is_plus", IsPlusUser);
    result.InsertValue("is_yandexoid", IsYandexoid);
    if (DeviceId) {
        result.InsertValue("device_id", DeviceId);
    }
    if (DeviceName) {
        result.InsertValue("device_name", DeviceName);
    }
    if (!Scopes.empty()) {
        result.InsertValue("scopes", NJson::ToJson(Scopes));
    }
    return result;
}

const TString& TBlackboxAuthInfo::GetUserId() const {
    return PassportUid ? PassportUid : ClientId;
}
