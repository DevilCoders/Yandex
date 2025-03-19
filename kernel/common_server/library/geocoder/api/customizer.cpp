#include "customizer.h"

#include <yandex/maps/proto/common2/response.pb.h>
#include <yandex/maps/proto/search/experimental.pb.h>
#include <yandex/maps/proto/search/geocoder_internal.pb.h>
#include <yandex/maps/proto/search/geocoder.pb.h>

#include <util/string/builder.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

namespace NExternalAPI {

    TGeocoderCustomizer::TFactory::TRegistrator<TGeocoderCustomizer> TGeocoderCustomizer::Registrator(TGeocoderCustomizer::GetTypeName());

    void TGeocoderCustomizer::DoToString(IOutputStream& os) const {
        os << "Path: " << Path << Endl;
        os << "Origin: " << Origin << Endl;
        os << "RevMode: " << RevMode << Endl;
    }

    void TGeocoderCustomizer::DoInit(const TYandexConfig::Section* section) {
        Path = section->GetDirectives().Value("Path", Path);
        Origin = section->GetDirectives().Value("Origin", Origin);
        RevMode = section->GetDirectives().Value("RevMode", RevMode);
    }

    bool TGeocoderCustomizer::DoTuneRequest(const IServiceApiHttpRequest& /*baseRequest*/, NNeh::THttpRequest& request, const IRequestCustomizationContext*, const NExternalAPI::TSender* sender) const {
        request.SetUri(Path);
        request.AddCgiData("origin", GetOrigin());
        if (!request.HasCgi("text")) {
            request.AddCgiData("geocoder_revmode", GetRevMode());
        }
        if (sender) {
            request.AddCgiData("timeout", sender->GetConfig().GetRequestConfig().GetGlobalTimeout().MicroSeconds());
        }
        return true;
    }

}
