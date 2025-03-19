#include "settings.h"

namespace NCS {
    namespace NPropositions {
        IProposedAction::TFactory::TRegistrator<TSettingsAction> RegistratorSettingsAction(TSettingsAction::GetTypeName());
    }
}
