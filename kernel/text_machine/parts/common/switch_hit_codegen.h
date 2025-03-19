#pragma once
#include <kernel/text_machine/module/interfaces.h>

namespace NTextMachine {
namespace NCore {

    class TSwitchHitCodegen
        : public NModule::IMethodCustomGenerator
        , public NModule::IMachineDefinitionListener
    {
    public:
        NModule::IMachineDefinitionListener* GetListener() override {
            return this;
        }
        void ProcessMachineDefinition(const NTextMachineParts::TCodeGenInput::TMachineDescriptor& machine) override;
        void Generate(IOutputStream& out, const NTextMachineParts::TCodeGenInput::TMachineDescriptor& machine, NModule::ICodegenHelpers& helpers) const override;
    private:
        THashMap<TString, TVector<int>> MachineUsesStreams;
    };

}
}
