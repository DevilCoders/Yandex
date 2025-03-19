#include "role.h"

namespace NCS {
    namespace NPropositions {
        IProposedAction::TFactory::TRegistrator<TRoleActionUpsert> RegistratorRoleAdd(TRoleActionUpsert::GetTypeName());
        IProposedAction::TFactory::TRegistrator<TRoleActionRemove> RegistratorRoleRemove(TRoleActionRemove::GetTypeName());
    }
}
