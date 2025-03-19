#pragma once

#include <kernel/common_server/library/geometry/coord.h>
#include <kernel/common_server/library/json/field.h>

#include <kernel/common_server/library/tvm_services/abstract/request/abstract.h>

#include <library/cpp/langs/langs.h>

class TGeocoderRequest: public NExternalAPI::IServiceApiHttpRequest {
private:
    const TMaybe<TGeoCoord> Coordinate;
    const ELanguage Language = LANG_RUS;
    const TMaybe<TString> TextAddress;
    const TString Type = "geo";
public:
    TGeocoderRequest(const TMaybe<TGeoCoord> coord, const ELanguage lang, const TMaybe<TString> textAddress = Nothing(), const TString& type = "geo")
        : Coordinate(coord)
        , Language(lang)
        , TextAddress(textAddress)
        , Type(type) {
    }

    bool BuildHttpRequest(NNeh::THttpRequest& request) const;

    struct TAddressDetails {
        TString Street;
        TString House;
        TString Province;
        TString Country;
        TString Locality;
        TString Region;
        TString Area;
        TString District;
        TString MetroStation;
        TString Entrance;
    };

    struct TGeoResponse {
        TString Description;
        TString Title;
        TString Kind;
        ui64 GeoId = 0;
        TAddressDetails Address;
    };

    class TResponse: public IResponse {
        CSA_READONLY_DEF(TGeoResponse, Details);
        CSA_READONLY_DEF(TGeoCoord, Coord);
    protected:
        virtual bool DoParseReply(const NUtil::THttpReply& reply) override;
    };
};
