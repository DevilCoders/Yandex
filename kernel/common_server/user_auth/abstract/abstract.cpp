#include "abstract.h"

bool IAuthUsersManager::AuthUserIdToInternalUserId(const TAuthUserLinkId& authId, TString& internalUserId) const {
    if (!DoAuthUserIdToInternalUserId(authId, internalUserId)) {
        return false;
    }
    if (!internalUserId) {
        internalUserId = Server.GetSettings().GetValueDef<TString>("users.default_user", "undefined");
    }
    return true;
}

bool IAuthUsersManager::Upsert(const TAuthUserLink& /*userData*/, const TString& /*userId*/, bool* /*isUpdate*/) const {
    return false;
}

bool IAuthUsersManager::Remove(const TVector<ui32>& /*linkIds*/, const TString& /*userId*/) const {
    return false;
}

bool IAuthUsersManager::Remove(const TVector<TAuthUserLinkId>& /*authIds*/, const TString& /*userId*/) const {
    return false;
}

bool IAuthUsersManager::RestoreAll(TVector<TAuthUserLink>& data) const {
    Y_UNUSED(data);
    return false;
}

NJson::TJsonValue TAuthUserLinkId::SerializeToJson() const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    result.InsertValue("auth_module_id", AuthModuleId);
    result.InsertValue("auth_user_id", AuthUserId);
    return result;
}

bool TAuthUserLinkId::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TJsonProcessor::Read(jsonInfo, "auth_user_id", AuthUserId, true)) {
        return false;
    }
    if (!TJsonProcessor::Read(jsonInfo, "auth_module_id", AuthModuleId)) {
        return false;
    }
    if (!AuthUserId) {
        TFLEventLog::Log("incorrect auth user id (empty)");
        return false;
    }
    return true;
}

TString TAuthUserLinkId::GetAuthIdString(const bool useAuthModuleId /*= true*/) const {
    if (!useAuthModuleId || !AuthModuleId) {
        return AuthUserId;
    } else {
        return AuthModuleId + ":" + AuthUserId;
    }
}

NFrontend::TScheme TAuthUserLinkId::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result;
    result.Add<TFSVariants>("auth_module_id").SetVariants(server.GetAuthModuleIds());
    result.Add<TFSString>("auth_user_id").SetNonEmpty(true);
    return result;
}

NJson::TJsonValue TAuthUserLink::SerializeToJson() const {
    NJson::TJsonValue result = TBase::SerializeToJson();
    result.InsertValue("system_user_id", SystemUserId);
    result.InsertValue("link_id", LinkId);
    result.InsertValue("_title", GetAuthIdString());
    return result;
}

bool TAuthUserLink::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    if (!TBase::DeserializeFromJson(jsonInfo)) {
        return false;
    }
    if (!TJsonProcessor::Read(jsonInfo, "system_user_id", SystemUserId)) {
        return false;
    }
    if (!TJsonProcessor::Read(jsonInfo, "link_id", LinkId)) {
        return false;
    }
    if (!SystemUserId) {
        TFLEventLog::Log("incorrect user name (empty)");
        return false;
    }
    return true;
}

NFrontend::TScheme TAuthUserLink::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result = TBase::GetScheme(server);
    result.Add<TFSString>("system_user_id").SetNonEmpty(true);
    result.Add<TFSNumeric>("link_id").SetReadOnly(true);
    result.Add<TFSString>("_title").SetReadOnly(true);
    return result;
}

NFrontend::TScheme TAuthUserLink::GetSearchScheme(const IBaseServer& server) {
    NFrontend::TScheme result;
    result.Add<TFSStructure>("auth_id").SetStructure(TBase::GetScheme(server), false);
    result.Add<TFSString>("system_user_id").SetRequired(false);
    return result;
}
