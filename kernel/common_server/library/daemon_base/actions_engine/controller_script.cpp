#include "controller_script.h"
#include "controller_client.h"

namespace NRTYScript {

    TSlotAction::TFactory::TRegistrator<TSlotAction> Registrator("slot_action");

    bool TSlotAction::Execute(void* externalInfo) const {
        if (!NDaemonController::TControllerAgent(Slot.FullHost(), Slot.Port, (NDaemonController::IControllerAgentCallback*)externalInfo, Slot.UriPrefix).ExecuteAction(*Action))
            return false;
        return !Action->IsFailed();
    }

}
