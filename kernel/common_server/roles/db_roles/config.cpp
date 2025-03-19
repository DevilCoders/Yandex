#include "config.h"
#include "manager.h"

TDBFullRolesManagerConfig::TFactory::TRegistrator<TDBFullRolesManagerConfig> TDBFullRolesManagerConfig::Registrator(TDBFullRolesManagerConfig::GetTypeName());

THolder<IRolesManager> TDBFullRolesManagerConfig::BuildManager(const IBaseServer& server) const {
    return MakeHolder<TDBFullRolesManager>(server, *this);
}
