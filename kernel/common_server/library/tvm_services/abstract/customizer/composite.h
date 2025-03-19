#pragma once
#include "abstract.h"

namespace NExternalAPI {

    class TCompositeRequestCustomizer: public IRequestCustomizer {
    private:
        using TBase = IRequestCustomizer;
        static TFactory::TRegistrator<TCompositeRequestCustomizer> Registrator;
    protected:
        virtual bool DoTuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request, const IRequestCustomizationContext* context, const TSender* sender) const override;
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        virtual bool DoStart(const TSender& client) override;
        virtual bool DoStop() override;
        TMap<TString, TRequestCustomizerContainer> Customizers;
        TVector<TString> OrderedCustomizers;
    public:
        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        static TString GetTypeName() {
            return "composite";
        }
    };

}
