#pragma once
#include "abstract.h"

namespace NExternalAPI {

    class TZoraRequestCustomization: public IRequestCustomizer {
    private:
        static TFactory::TRegistrator<TZoraRequestCustomization> Registrator;
        CSA_READONLY_DEF(TString, DestUrl);
        CSA_READONLY_DEF(TString, ClientId);

    protected:
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        virtual bool DoTuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request, const IRequestCustomizationContext*, const TSender* /*sender*/) const override;
    public:
        static TString GetTypeName() {
            return "zora";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };
}
