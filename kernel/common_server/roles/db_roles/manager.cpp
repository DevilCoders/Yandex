#include "manager.h"

bool TDBFullRolesManager::DoStart() {
    if (!TBase::DoStart()) {
        return false;
    }
    if (!ItemsManager->Start()) {
        return false;
    }
    if (!RolesManager->Start()) {
        return false;
    }
    if (!RoleRoleLinksManager->Start()) {
        return false;
    }
    if (!RoleItemLinksManager->Start()) {
        return false;
    }
    return true;
}

bool TDBFullRolesManager::DoStop() {
    if (!RoleItemLinksManager->Stop()) {
        return false;
    }
    if (!RoleRoleLinksManager->Stop()) {
        return false;
    }
    if (!RolesManager->Stop()) {
        return false;
    }
    if (!ItemsManager->Stop()) {
        return false;
    }
    if (!TBase::DoStop()) {
        return false;
    }
    return true;
}
