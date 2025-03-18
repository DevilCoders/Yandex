#pragma once

#include <library/cpp/reverse_geocoder/core/location.h>
#include <library/cpp/reverse_geocoder/proto/region.pb.h>
#include <util/generic/strbuf.h>

namespace NXml {
    class TTextReader;
}

namespace NReverseGeocoder {
    namespace NYandexMap {
        typedef TVector<TStringBuf> StrBufList;

        struct ToponymTraits {
            ToponymTraits(NXml::TTextReader& xmlReader, const StrBufList& uselessKindList, const TVector<TString>& uselessElementsList, bool onlyOwner = false);

            long Id{};
            long ParentId{};
            long SourceId{-1};

            bool Skip{};
            bool IsOwner{};
            bool IsSquarePoly{}; // 9/metro, 14/monorail; TODO(dieash@) extract to external config?

            TString Kind;
            TString AddrRuName;
            TString AddrEnName;
            TString OfficialRuName;
            TString URI;

            TLocation CenterPos;
            NProto::TRegion Region;
            size_t BordersQty{};
            size_t PointsQty{};
            size_t Square{};

            TString ToString() const;
        };
    }
}
