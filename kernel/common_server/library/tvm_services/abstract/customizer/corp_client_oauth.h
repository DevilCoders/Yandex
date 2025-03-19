#pragma once
#include "abstract.h"

namespace NExternalAPI {

    class TCorpClientOAuthRequestCustomization: public NExternalAPI::IRequestCustomizer {
    private:
        using TBase = NExternalAPI::IRequestCustomizer;
        static TFactory::TRegistrator <TCorpClientOAuthRequestCustomization> Registrator;
    protected:
        virtual void DoInit(const TYandexConfig::Section* section) override;
        virtual void DoToString(IOutputStream& os) const override;
        virtual bool DoTuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request, const NExternalAPI::IRequestCustomizationContext*, const NExternalAPI::TSender* /*sender*/) const override;
    public:
        static TString GetTypeName() {
            return "corp_client_oauth";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        TString MaskToken(const TString& token) const;
        TVector<TString> GetCorpClientIds() const;

    private:
        TString DefaultToken;
        TMap<TString, TString> CorpClientToken;
    };

}
