#pragma once

#include <library/cpp/reverse_geocoder/core/common.h>
#include <library/cpp/reverse_geocoder/open_street_map/proto/open_street_map.pb.h>
#include <util/generic/vector.h>

namespace NReverseGeocoder {
    namespace NOpenStreetMap {
        struct TKv {
            TString K;
            TString V;
        };

        struct TReference {
            enum EType {
                TYPE_NODE = NProto::TRelation::NODE,
                TYPE_WAY = NProto::TRelation::WAY,
                TYPE_RELATION = NProto::TRelation::RELATION,
            };

            TGeoId GeoId;
            EType Type;
            TString Role;
        };

        typedef TVector<TKv> TKvs;
        typedef TVector<TReference> TReferences;
        typedef TVector<TGeoId> TGeoIds;

        TString FindName(const TKvs& kvs);

    }
}
