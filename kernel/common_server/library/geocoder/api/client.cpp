#include "client.h"

#include <yandex/maps/proto/common2/response.pb.h>
#include <yandex/maps/proto/search/experimental.pb.h>
#include <yandex/maps/proto/search/geocoder_internal.pb.h>
#include <yandex/maps/proto/search/geocoder.pb.h>

#include <util/string/builder.h>

namespace NPb = ::yandex::maps::proto;

namespace {
    bool FillAddressFromPb(TGeocoderRequest::TAddressDetails& address, const NPb::search::geocoder::GeoObjectMetadata& metadata) {
        using EKind = NPb::search::kind::Kind;

        for (const auto& component : metadata.address().component()) {
            if (component.kind().empty()) {
                continue;
            }

            const auto kind = component.kind(0);
            const auto& name = component.name();

            if (kind == EKind::COUNTRY) {
                address.Country = name;
            } else if (kind == EKind::REGION) {
                address.Region = name;
            } else if (kind == EKind::PROVINCE) {
                address.Province = name;
            } else if (kind == EKind::AREA) {
                address.Area = name;
            } else if (kind == EKind::LOCALITY) {
                address.Locality = name;
            } else if (kind == EKind::DISTRICT) {
                address.District = name;
            } else if (kind == EKind::STREET) {
                address.Street = name;
            } else if (kind == EKind::HOUSE) {
                address.House = name;
            } else if (kind == EKind::METRO_STATION) {
                address.MetroStation = name;
            } else if (kind == EKind::ENTRANCE) {
                address.Entrance = name;
            }
        }

        if (!address.Locality) {
            address.Locality = address.Province;
        }

        return true;
    }
}

bool TGeocoderRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
    if (Coordinate) {
        request.AddCgiData("ll", TStringBuilder() << Coordinate->X << ',' << Coordinate->Y).Encode(); // lon,lat
    }
    request.AddCgiData("lang", IsoNameByLanguage(Language));

    request.AddCgiData("type", Type);

    request.AddCgiData("gta", "kind");
    request.AddCgiData("gta", "ll");

    if (TextAddress.Defined()) {
        request.AddCgiData("text", *TextAddress).Encode();
    } else {
        request.AddCgiData("mode", "reverse");
    }
    request.AddCgiData("ms", "pb");
    return true;
}


bool TGeocoderRequest::TResponse::DoParseReply(const NUtil::THttpReply& rawReply) {
    auto gLogging = TFLRecords::StartContext()("reply_id", "");
    if (rawReply.Code() != HTTP_OK) {
        TFLEventLog::Log("request failed")("code", rawReply.Code());
        return false;
    }

    NPb::common2::response::Response response;
    if (!response.ParseFromString(rawReply.Content())) {
        TFLEventLog::Log("cannot parse response");
        return false;
    }

    const auto& reply = response.reply();
    if (reply.geo_object().empty()) {
        TFLEventLog::Log("documents empty");
        return false;
    }

    const auto& object = reply.geo_object(0);
    Details.Title = object.name();
    Details.Description = object.description();

    bool metadataFound = false;
    for (const auto& commonMetadata : object.metadata()) {
        if (commonMetadata.HasExtension(NPb::search::geocoder::GEO_OBJECT_METADATA)) {
            metadataFound = true;

            const auto& metadata = commonMetadata.GetExtension(NPb::search::geocoder::GEO_OBJECT_METADATA);
            FillAddressFromPb(Details.Address, metadata);

            if (metadata.HasExtension(NPb::search::geocoder_internal::TOPONYM_INFO)) {
                const auto& internal = metadata.GetExtension(NPb::search::geocoder_internal::TOPONYM_INFO);
                Details.GeoId = internal.geoid();
            }
        }
        if (commonMetadata.HasExtension(NPb::search::experimental::GEO_OBJECT_METADATA)) {
            const auto& metadata = commonMetadata.GetExtension(NPb::search::experimental::GEO_OBJECT_METADATA);
            for (const auto& item : metadata.experimental_storage().item()) {
                if (item.key() == "kind") {
                    Details.Kind = item.value();
                } else if (item.key() == "ll") {
                    if (!Coord.DeserializeFromStringLonLat(item.value(), ',')) {
                        TFLEventLog::Error("GeocoderRequest coordinate bad deserialization");
                    }
                }
            }
        }
    }

    if (!metadataFound) {
        TFLEventLog::Log("geocoder metadata not found");
        return false;
    }

    return true;
}
