#pragma once

#include <kernel/common_server/library/tvm_services/abstract/customizer/abstract.h>

#include <library/cpp/langs/langs.h>

namespace NExternalAPI {

    class TGeocoderCustomizer: public IRequestCustomizer {
    private:
        CSA_READONLY(TString, Path, "search/stable/yandsearch");
        CSA_READONLY(TString, Origin, "cs");
        CSA_READONLY(TString, RevMode, "taxi2");
    protected:
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        virtual bool DoTuneRequest(const IServiceApiHttpRequest& /*baseRequest*/, NNeh::THttpRequest& request, const IRequestCustomizationContext*, const TSender* sender) const override;
    public:
        static TString GetTypeName() {
            return "geocoder";
        }
        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    private:
        static TFactory::TRegistrator<TGeocoderCustomizer> Registrator;
    };

}
