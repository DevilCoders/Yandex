#include "toponym_traits.h"

#include "polygon_parser.h"

#include <library/cpp/reverse_geocoder/core/common.h>
#include <library/cpp/reverse_geocoder/generator/common.h>
#include <library/cpp/reverse_geocoder/logger/log.h>
#include <library/cpp/xml/document/xml-textreader.h>

#include <util/string/split.h>
#include <util/system/yassert.h>

using namespace NReverseGeocoder;
using namespace NReverseGeocoder::NYandexMap;

namespace {
    bool IsUselessKind(TStringBuf checkedKind, const StrBufList& uselessKindList) {
        return uselessKindList.size() && uselessKindList.end() != std::find(uselessKindList.begin(), uselessKindList.end(), checkedKind);
    }

    bool IsBeginElement(const NXml::TTextReader& reader, const char* name) {
        return reader.GetNodeType() == NXml::TTextReader::ENodeType::Element && reader.GetName() == name;
    }

    bool IsEndElement(const NXml::TTextReader& reader, const char* name) {
        return reader.GetNodeType() == NXml::TTextReader::ENodeType::EndElement && reader.GetName() == name;
    }

    char const * const ID_ELEM = "id";
    char const * const PID_ELEM = "parent";
    char const * const GID_ELEM = "geoid";
    char const * const SID_ELEM = "sourceId";

    bool IsId(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, ID_ELEM);
    }

    bool IsParentId(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, PID_ELEM);
    }

    bool IsSourceId(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, SID_ELEM);
    }

    bool IsGeoId(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, GID_ELEM);
    }

    bool IsEndGeoId(const NXml::TTextReader& reader) {
        return IsEndElement(reader, GID_ELEM);
    }

    bool IsKind(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "kind");
    }

    bool IsEndKind(const NXml::TTextReader& reader) {
        return IsEndElement(reader, "kind");
    }

    bool IsURI(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "URI");
    }

    bool IsEndURI(const NXml::TTextReader& reader) {
        return IsEndElement(reader, "URI");
    }

    bool IsAddr(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "address");
    }

    bool IsName(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "name");
    }

    bool IsGmlPoint(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "gml:pos");
    }

    bool IsEndGmlPoint(const NXml::TTextReader& reader) {
        return IsEndElement(reader, "gml:pos");
    }

    bool IsToponym(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "Toponym");
    }

    bool IsEndToponym(const NXml::TTextReader& reader) {
        return IsEndElement(reader, "Toponym");
    }

    bool IsText(const NXml::TTextReader& reader) {
        return reader.GetNodeType() == NXml::TTextReader::ENodeType::Text;
    }

    bool IsGeoIdOwner(const NXml::TTextReader& reader) {
        const TString OWNER = "owner";
        return FromString<bool>(reader.GetAttribute(OWNER));
    }

    bool IsAddr(const NXml::TTextReader& reader, const char* lang) {
        return IsAddr(reader)
            && lang == reader.GetAttribute("xml:lang");
    }

    bool IsNameRuOfficial(const NXml::TTextReader& reader) {
        return IsName(reader)
            && "ru" == reader.GetAttribute("locale")
            && "official" == reader.GetAttribute("type");
    }

    void ParseGeoId(NXml::TTextReader& reader, NReverseGeocoder::NProto::TRegion& region, bool& isOwner) {
        isOwner = IsGeoIdOwner(reader);
        while (reader.Read()) {
            if (IsText(reader))
                region.SetRegionId(FromString<TGeoId>(reader.GetValue()));
            if (IsEndGeoId(reader))
                break;
        }
    }

    TStringBuf GetKind(NXml::TTextReader& reader) {
        while (reader.Read()) {
            if (IsText(reader))
                return reader.GetValue();
            if (IsEndKind(reader))
                break;
        }

        throw std::runtime_error("unable to detect kind");
    }

    TStringBuf GetURI(NXml::TTextReader& reader) {
        while (reader.Read()) {
            if (IsText(reader))
                return reader.GetValue();
            if (IsEndURI(reader))
                break;
        }

        throw std::runtime_error("unable to detect URI");
    }

    long ExtractInteger(NXml::TTextReader& reader, char const * elemName) {
        while (reader.Read()) {
            if (IsText(reader))
                return FromString<long>(reader.GetValue());
            if (IsEndElement(reader, elemName))
                break;
        }
        ythrow yexception() << "unable to get " << elemName;
    }

    long GetParentId(NXml::TTextReader& reader) {
        return ExtractInteger(reader, PID_ELEM);
    }

    long GetId(NXml::TTextReader& reader) {
        return ExtractInteger(reader, ID_ELEM);
    }

    long GetSourceId(NXml::TTextReader& reader) {
        return ExtractInteger(reader, SID_ELEM);
    }

    TStringBuf ExtractString(NXml::TTextReader& reader, char const * elemName) {
        while (reader.Read()) {
            if (IsText(reader))
                return reader.GetValue();
            if (IsEndElement(reader, elemName))
                break;
        }
        ythrow yexception() << "unable to get " << elemName;
    }

    TStringBuf GetAddr(NXml::TTextReader& reader) {
        return ExtractString(reader, "address");
    }

    TStringBuf GetNameRuOfficial(NXml::TTextReader& reader) {
        return ExtractString(reader, "name");
    }

    TLocation GetGmlPoint(NXml::TTextReader& reader) {
        TStringBuf pos;
        while (reader.Read()) {
            if (IsText(reader))
                pos = reader.GetValue();
            break;
            if (IsEndGmlPoint(reader))
                break;
        }

        if (pos.empty()) {
            throw std::runtime_error("unable to get center-point");
        }

        StrBufList posParts; // idx0/1 => lon/lat
        StringSplitter(pos).Split(' ').AddTo(&posParts);
        Y_ASSERT(2 == posParts.size());
        return {FromString<double>(posParts[0]), FromString<double>(posParts[1])};
    }

    void SkipElement(NXml::TTextReader& reader, const TStringBuf elemName) {
        while (reader.Read()) {
            if (IsEndElement(reader, elemName.Data())) {
                break;
            }
        }
    }

    void SkipToponym(NXml::TTextReader& reader) {
        SkipElement(reader, "Toponym");
    }

    bool IsGeometry(const NXml::TTextReader& reader) {
        return IsBeginElement(reader, "geometry");
    }

    bool IsEndGeometry(const NXml::TTextReader& reader) {
        return IsEndElement(reader, "geometry");
    }

    void ParsePolygon(const TStringBuf& str, NProto::TRegion& region) {
        TPolygonParser parser(region);
        parser.Parse(str);
    }

    void ParseMultiPolygon(const TStringBuf& str, NProto::TRegion& region) {
        TMultiPolygonParser parser(region);
        parser.Parse(str);
    }

    void ParseGeometryImpl(const TStringBuf& geometry, NProto::TRegion& region) {
        const TStringBuf POLYGON = "POLYGON";
        if (geometry.StartsWith(POLYGON))
            ParsePolygon(geometry.SubStr(POLYGON.size()), region);

        const TStringBuf MULTIPOLYGON = "MULTIPOLYGON";
        if (geometry.StartsWith(MULTIPOLYGON))
            ParseMultiPolygon(geometry.SubStr(MULTIPOLYGON.size()), region);
    }

    void ParseGeometry(NXml::TTextReader& reader, NProto::TRegion& region) {
        while (reader.Read()) {
            if (IsText(reader))
                ParseGeometryImpl(reader.GetValue(), region);
            if (IsEndGeometry(reader))
                break;
        }
    }

    typedef std::vector<TLocation> SquarePolyType;

    SquarePolyType BuildSquareBorders(const TLocation& center) {
        const double Size50Meters = 0.0005;
        const SquarePolyType square_poly{
            {center.Lon - Size50Meters, center.Lat - Size50Meters},
            {center.Lon + Size50Meters, center.Lat - Size50Meters},
            {center.Lon + Size50Meters, center.Lat + Size50Meters},
            {center.Lon - Size50Meters, center.Lat + Size50Meters}};
        return square_poly;
    }

    void AddSquareBorders(NProto::TRegion& region, const SquarePolyType& poly) {
        NProto::TPolygon* pPoly = region.AddPolygons();
        pPoly->SetType(NProto::TPolygon::TYPE_OUTER);
        pPoly->SetPolygonId(0);

        for (const auto& point : poly) {
            NProto::TLocation* pLoc = pPoly->AddLocations();
            pLoc->SetLon(point.Lon);
            pLoc->SetLat(point.Lat);
        }
    }

    size_t CalcSquareOfProtoPolygon(const NProto::TPolygon& polygon) {
        TVector<TPoint> list;
        list.reserve(polygon.LocationsSize());

        for (const auto& point : polygon.GetLocations()) {
            list.push_back(TPoint(TLocation(point.GetLon(), point.GetLat())));
        }

        return NGenerator::GetSquare(list);
    }

    bool IsUselessDetectedAndSkipped(NXml::TTextReader& xmlReader, const TVector<TString>& uselessElements) {
        for (const auto& useless : uselessElements) {
            if (IsBeginElement(xmlReader, useless.Data())) {
                SkipElement(xmlReader, useless);
                return true;
            }
        }
        return false;
    }
}  // anon-ns

namespace NReverseGeocoder::NYandexMap {
ToponymTraits::ToponymTraits(
    NXml::TTextReader& xmlReader, const StrBufList& uselessKindList, const TVector<TString>& uselessElementsList, bool onlyOwner) {
    do {
        if (IsToponym(xmlReader)) {
            break;
        }
    } while (xmlReader.Read());

    while (xmlReader.Read()) {
        if (IsEndToponym(xmlReader)) {
            break;
        }

        if (Skip) {
            SkipToponym(xmlReader);
            break;
        }

        if (IsUselessDetectedAndSkipped(xmlReader, uselessElementsList)) {
            continue;
        }

        if (IsId(xmlReader)) {
            long id = GetId(xmlReader);
            if (Id) {
                ythrow yexception() << "we already have ID#" << Id << "; but receive another one #" << id;
            }
            Id = id;
        }
        if (IsParentId(xmlReader)) {
            ParentId = GetParentId(xmlReader);
        }
        if (IsSourceId(xmlReader)) {
            SourceId = GetSourceId(xmlReader);
        }
        if (IsKind(xmlReader)) {
            Kind = GetKind(xmlReader);
            Skip = IsUselessKind(Kind, uselessKindList);
            if (Skip) {
                continue;
            }
            IsSquarePoly = ("metro" == Kind);
        }
        if (IsURI(xmlReader)) {
            URI = GetURI(xmlReader);
        }
        if (IsGeoId(xmlReader)) {
            ParseGeoId(xmlReader, Region, IsOwner);
            Skip = !IsOwner && onlyOwner;
            if (Skip) {
                continue;
            }
        }
        if (IsGmlPoint(xmlReader)) {
            CenterPos = GetGmlPoint(xmlReader);
        }
        if (IsNameRuOfficial(xmlReader)) {
            OfficialRuName = GetNameRuOfficial(xmlReader);
        }
        if (IsAddr(xmlReader, "en")) {
            AddrEnName = GetAddr(xmlReader);
        }
        if (IsAddr(xmlReader, "ru")) {
            AddrRuName = GetAddr(xmlReader);

            if (("station" == Kind) && AddrRuName.Contains("монорельс")) {
                Skip = false;
                IsSquarePoly = true;
            }
            if (IsSquarePoly && (CenterPos.Lat || CenterPos.Lon)) {
                AddSquareBorders(Region, BuildSquareBorders(CenterPos));
            }
        }
        if (!IsSquarePoly && IsGeometry(xmlReader)) {
            ParseGeometry(xmlReader, Region);
        }
    }

    // quick-stats
    BordersQty = Region.PolygonsSize();
    for (const NProto::TPolygon& polygon : Region.GetPolygons()) {
        PointsQty += polygon.LocationsSize();
        Square += CalcSquareOfProtoPolygon(polygon);
     }
}

TString ToponymTraits::ToString() const {
    TStringStream out;
    out << "toponym#" << Id << "\t" << Kind << "\t" << Kind << "\t" << AddrRuName;
    return out.Str();
}

}  // NYandexMap
