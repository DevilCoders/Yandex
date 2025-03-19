#pragma once
#include "abstract.h"
#include <kernel/common_server/library/interfaces/tvm_manager.h>
#include <library/cpp/tvmauth/client/facade.h>

namespace NExternalAPI {

    class TTVMRequestCustomization: public NExternalAPI::IRequestCustomizer {
        CSA_READONLY(ui32, DestinationClientId, 0);
        CSA_READONLY_DEF(TString, TvmClientName);
        CSA_READONLY_DEF(TString, TicketString);
        CSA_READONLY_DEF(TString, Token);
    private:
        using TBase = NExternalAPI::IRequestCustomizer;
        static TFactory::TRegistrator <TTVMRequestCustomization> Registrator;
    protected:
        virtual void DoInit(const TYandexConfig::Section* /*section*/) override;
        virtual void DoToString(IOutputStream& /*os*/) const override;
        virtual bool DoTuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request, const NExternalAPI::IRequestCustomizationContext* cContext, const NExternalAPI::TSender* sender) const override;
    public:
        static TString GetTypeName() {
            return "tvm";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };

}
