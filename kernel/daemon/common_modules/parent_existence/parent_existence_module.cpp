#include "parent_existence_module.h"

namespace NParentExistenceChecker {
    IDaemonModuleConfig::TFactory::TRegistrator<TParentExistenceConfig> TParentExistenceConfig::Registrator(PARENT_EXISTENCE_CHECKER_MODULE_NAME);
    TDaemonModules::TFactory::TRegistrator<TParentExistenceChecker> TParentExistenceChecker::Registrator(PARENT_EXISTENCE_CHECKER_MODULE_NAME);
}
