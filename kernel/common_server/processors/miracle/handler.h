#pragma once
#include <kernel/common_server/processors/common/handler.h>

namespace NCS {
    class TMiracleHandler: public TCommonSystemHandler<TMiracleHandler, TEmptyConfig> {
    private:
        using TBase = TCommonSystemHandler<TMiracleHandler, TEmptyConfig>;
    public:
        using TBase::TBase;

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) final;
        static TString GetTypeName() {
            return "miracle";
        }
    };
}
