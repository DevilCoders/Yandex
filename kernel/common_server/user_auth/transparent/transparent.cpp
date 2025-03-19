#include "transparent.h"

TTransparentAuthUsersManagerConfig::TFactory::TRegistrator<TTransparentAuthUsersManagerConfig> TTransparentAuthUsersManagerConfig::Registrator(TTransparentAuthUsersManagerConfig::GetTypeName());

IAuthUsersManager::TPtr TTransparentAuthUsersManagerConfig::BuildManager(const IBaseServer& server) const {
    return MakeAtomicShared<TTransparentAuthUsersManager>(server);
}

bool TTransparentAuthUsersManager::Restore(const TAuthUserLinkId& authId, TMaybe<TAuthUserLink>& data) const {
    TAuthUserLink result(authId);
    result.SetSystemUserId(authId.GetAuthUserId());
    data = result;
    return true;
}
