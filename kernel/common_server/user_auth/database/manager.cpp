#include "manager.h"

TSRCondition TDBAuthUsersManager::BuildAuthIdRequest(const TAuthUserLinkId& authId) const {
    TSRCondition result;
    auto& rMulti = result.RetObject<TSRMulti>();
    rMulti.InitNode<TSRBinary>("auth_user_id", authId.GetAuthUserId());
    if (Config.GetUseAuthModuleId()) {
        if (!!authId.GetAuthModuleId()) {
            rMulti.InitNode<TSRBinary>("auth_module_id", authId.GetAuthModuleId());
        } else {
            rMulti.InitNode<TSRNullChecker>("auth_module_id");
        }
    }
    return result;
}

bool TDBAuthUsersManager::DoAuthUserIdToInternalUserId(const TAuthUserLinkId& authId, TString& userId) const {
    TVector<TDBAuthUserLink> links;
    {
        TReadGuard rg(MutexCachedObjects);
        links = IndexByAuthId.GetObjectsByKey(authId.GetAuthIdString(Config.GetUseAuthModuleId()));
    }
    if (links.size() == 1) {
        userId = links.front().GetSystemUserId();
    } else if (links.size() == 0) {
        userId = "";
    } else {
        return false;
    }
    return true;
}

bool TDBAuthUsersManager::Upsert(const TAuthUserLink& userData, const TString& userId, bool* isUpdate) const {
    auto session = BuildNativeSession(false);
    NStorage::TTableRecord trCondition;
    if (userData.GetLinkId()) {
        trCondition.Set("link_id", userData.GetLinkId());
    } else {
        trCondition.Set("auth_module_id", userData.GetAuthModuleId());
        trCondition.Set("auth_user_id", userData.GetAuthUserId());
    }
    return UpsertRecord(TDBAuthUserLink(userData).SerializeToTableRecord(), trCondition, userId, session, nullptr, isUpdate) && session.Commit();
}

bool TDBAuthUsersManager::Remove(const TVector<ui32>& linkIds, const TString& userId) const {
    if (linkIds.empty()) {
        return true;
    }
    auto session = BuildNativeSession(false);
    return RemoveObjectsBySRCondition(
        TSRCondition().Init<TSRBinary>("link_id", linkIds), userId, session) && session.Commit();
}

bool TDBAuthUsersManager::Remove(const TVector<TAuthUserLinkId>& authUserIds, const TString& userId) const {
    if (authUserIds.empty()) {
        return true;
    }
    auto session = BuildNativeSession(false);
    TSRCondition condition;
    auto& reqMulti = condition.RetObject<TSRMulti>();
    for (auto&& i : authUserIds) {
        reqMulti.MutableNodes().emplace_back(BuildAuthIdRequest(i));
    }
    return RemoveObjectsBySRCondition(condition, userId, session) && session.Commit();
}

bool TDBAuthUsersManager::Restore(const TString& systemUserId, TVector<TAuthUserLink>& data) const {
    auto session = BuildNativeSession(true);
    TVector<TDBAuthUserLink> result;
    if (!RestoreObjectsBySRCondition(result, TSRCondition().Init<TSRBinary>("user_name", systemUserId), session)) {
        return false;
    }
    TVector<TAuthUserLink> dataResult;
    for (auto&& i : result) {
        dataResult.emplace_back(i);
    }
    std::swap(data, dataResult);
    return true;
}

bool TDBAuthUsersManager::Restore(const TAuthUserLinkId& authId, TMaybe<TAuthUserLink>& data) const {
    auto session = BuildNativeSession(true);
    TMaybe<TDBAuthUserLink> result;
    if (!RestoreObjectBySRCondition(BuildAuthIdRequest(authId), result, session)) {
        return false;
    }
    data = result;
    return true;
}

bool TDBAuthUsersManager::RestoreAll(TVector<TAuthUserLink>& data) const {
    auto session = BuildNativeSession(true);
    TVector<TDBAuthUserLink> result;
    if (!RestoreAllObjects(result, session)) {
        return false;
    }
    for (auto&& i : result) {
        data.emplace_back(i);
    }
    return true;
}
