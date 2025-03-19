#pragma once

#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class TServiceEmulationProcessor: public TCommonSystemHandler<TServiceEmulationProcessor> {
    private:
        using TBase = TCommonSystemHandler<TServiceEmulationProcessor>;
    public:
        using TBase::TBase;
        static TString GetTypeName() {
            return "service-emulation";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

}
