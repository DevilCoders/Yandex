#include "mset.h"

namespace NHtmlDetect {
    void TMachineSet::PushState() {
        for (auto& machine : Machines)
            machine.PushState();
    }
    void TMachineSet::PopState() {
        for (auto& machine : Machines)
            machine.PopState();
    }
    void TMachineSet::PushText(const char* text, size_t len) {
        for (auto& machine : Machines)
            machine.GetMachine().Push(text, len);
    }
    const TDetectResult& TMachineSet::GetResult() {
        UpdateResult();
        return Result;
    }

    void TMachineSet::UpdateResult() {
        for (auto& machine : Machines) {
            const TDetectResult& r = machine.GetMachine().GetResult();
            for (const auto& j : r)
                Result.insert(j);
        }
    }

    TMachineSet::TMachineSet() {
        for (size_t i = 0; i < TMachine::NumMachines; ++i) {
            Machines.push_back(TContext(i));
        }
    }

}
