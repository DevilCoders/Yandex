#include "zora.h"

namespace NExternalAPI {

    TZoraRequestCustomization::TFactory::TRegistrator<TZoraRequestCustomization> TZoraRequestCustomization::Registrator(TZoraRequestCustomization::GetTypeName());

    void TZoraRequestCustomization::DoInit(const TYandexConfig::Section* section) {
        const auto& dirs = section->GetDirectives();

        dirs.GetValue("DestUrl", DestUrl);
        AssertCorrectConfig(!!DestUrl, "Zora Customization requires valid URL in DestUrl parameter.");
        AssertCorrectConfig(DestUrl.find('#') == TString::npos, "Zora Customization does not support URL fragment component in DestUrl parameter.");
        AssertCorrectConfig(DestUrl.find('?') == TString::npos, "Zora Customization does not support URL query component in DestUrl parameter.");

        dirs.GetValue("ClientId", ClientId);
    }

    void TZoraRequestCustomization::DoToString(IOutputStream& os) const {
        os << "DestUrl: " << DestUrl << Endl;
        os << "ClientId: " << ClientId << Endl;
    }

    bool TZoraRequestCustomization::DoTuneRequest(const IServiceApiHttpRequest& /*baseRequest*/, NNeh::THttpRequest& request, const IRequestCustomizationContext*, const TSender* /*sender*/) const {
        const TString requestQuery = request.GetCgiData();
        request.AddHeader("X-Ya-Dest-Url", DestUrl + request.GetUri() + (requestQuery.empty() ? TString{} : "?" + requestQuery));
        request.AddHeader("X-Ya-Client-Id", ClientId);

        request.SetCgiData("");
        request.SetUri("");
        return true;
    }

}
