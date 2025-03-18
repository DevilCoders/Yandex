#include "toponym_checker.h"

#include "toponym_traits.h"

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/reverse_geocoder/logger/log.h>

#include <util/system/yassert.h>

using namespace NReverseGeocoder;
using namespace NYandexMap;

ToponymChecker::ToponymChecker()
    : MapIt(Mapping.end())
{
}

ToponymChecker::~ToponymChecker() {
}

void ToponymChecker::ShowReminder() const {
    INFO_LOG << "geomatch reminder: " << Mapping.size() << "\n";
    for (const auto& pair : Mapping) {
        Cout << pair.second.ToString() << "\n";
    }
}

#define TRAITS(t) ": #" << t.Region.GetRegionId() << "\t" << t.AddrRuName << "\t" << t.Kind << "\t" << t.Id << "\t" << t.BordersQty << "\t" << t.PointsQty << "\n";

bool ToponymChecker::IsToponymOK(ToponymTraits& traits) {
    if (traits.Skip) {
        return false;
    }

    if (!traits.Region.PolygonsSize()) {
        WARNING_LOG << "NO_POLY" << TRAITS(traits);
        return false;
    }

    return IsToponymOkImpl(traits);
}

bool ToponymChecker::IsToponymOkImpl(ToponymTraits& traits) {
    if (traits.IsSquarePoly) {
        DEBUG_LOG << "SQUARE_POLY" << TRAITS(traits);
    }

    const bool gidFound = SearchId(traits.Region.GetRegionId());
    if (traits.IsOwner && gidFound) {
        if (!IsKindOk(traits.Kind)) {
            ERROR_LOG << "KIND_DIFF" << TRAITS(traits);
            return false;
        }
        if ("country" == traits.Kind || IsNameOk(traits.AddrRuName)) {
            RemoveCurIt();
            INFO_LOG << "OK" << TRAITS(traits);
            return true;
        }
        ERROR_LOG << "NAME_DIFF" << TRAITS(traits);
    }

    if (traits.IsOwner && !gidFound) {
        ERROR_LOG << "OWN_NO_ID" << TRAITS(traits);
    }
    if (!gidFound) {
        ERROR_LOG << "NO_ID" << TRAITS(traits);
    }
    if (!SearchName(traits.AddrRuName)) {
        ERROR_LOG << "NO_NAME" << TRAITS(traits);
        return false;
    }

    traits.Region.SetRegionId(GetMappingTraits().RegionId);
    RemoveCurIt();
    INFO_LOG << "SUBST_OK" << TRAITS(traits);
    return true;
}

#undef TRAITS

void ToponymChecker::InsertItem(const TMappingTraits& traits) {
    Mapping[traits.RegionId] = traits;
}

bool ToponymChecker::SearchId(int id) {
    MapIt = Mapping.find(id);
    return Mapping.end() != MapIt;
}

void ToponymChecker::RemoveCurIt() {
    Mapping.erase(MapIt);
    MapIt = Mapping.end();
}

bool ToponymChecker::IsKindOk(const TString& kind) const {
    return (kind == GetMappingTraits().Kind);
}

bool ToponymChecker::IsNameOk(const TString& name) const {
    return (name == GetMappingTraits().ToponymPath);
}

const TMappingTraits& ToponymChecker::GetMappingTraits() const {
    if (Mapping.end() != MapIt) {
        return MapIt->second;
    }
    throw std::invalid_argument("invalid iterator state");
}

CheckerViaGeomapping::CheckerViaGeomapping(TStringBuf geoMappingFileName) {
    // NB: based on preliminary geomapping (because there are problems in geodata/geocode)
    Mapping = LoadGeoMapping(geoMappingFileName, SourceId2RegIdMap, ToponymId2RegIdMap);
    assert(Mapping.size());
    assert(SourceId2RegIdMap.size());
    assert(ToponymId2RegIdMap.size());
}

int CheckerViaGeomapping::FindMatchedRegionId(const ToponymTraits& traits) const {
    if (-1 != traits.SourceId) {
        const auto& it = SourceId2RegIdMap.find(traits.SourceId);
        if (SourceId2RegIdMap.end() != it) {
            const auto [ srcId, regId ] = *it;
            return regId;
        }
    }

    if (traits.Id < 0) {
        return -1;
    }

    if (0 == traits.Id) {
        WARNING_LOG << ">>> NOTA BENE: top-id in FindMatchedRegionId() eq ZERO // " << traits.ToString() << "\n";
    }

    const auto& it = ToponymId2RegIdMap.find(traits.Id);
    if (ToponymId2RegIdMap.end() != it) {
        const auto [ topId, regId ] = *it;
        return regId;
    }

    return -1;
}

bool CheckerViaGeomapping::IsToponymOkImpl(ToponymTraits& traits) {
    const int matchedRegId = FindMatchedRegionId(traits);
    if (-1 == matchedRegId) {
        return false;
    }

    if (SearchId(matchedRegId)) {
        traits.Region.SetRegionId(matchedRegId);
        RemoveCurIt();
        return true;
    }

    ythrow yexception() << "second usage of reg_id#" << matchedRegId << " via " << traits.ToString();
}

bool CheckerViaGeomapping::SearchName(TStringBuf name) {
    ythrow yexception() << "why you here? name: " << name;
}

namespace {
    typedef std::map<int, TStringBuf> Type2KindMapType;

    const Type2KindMapType type2KindMapping =
        { // NB: based on statistics; $ awk '{ print $4(type), $5(kind) }' data/*.geomatch | sort | uniq -c
            {3, "country"},
            {4, "province"},
            {5, "province"},
            {6, "locality"},
            {7, "locality"},
            {8, "district"},
            {9, "metro"},
            {10, "area"},
            {11, "airport"},
            {13, "district"},
            {14, "station"},
            {15, "area"}};

    TStringBuf GetKind(int type) {
        Type2KindMapType::const_iterator it = type2KindMapping.find(type);
        return type2KindMapping.end() == it ? TStringBuf() : it->second;
    }
}

CheckerViaGeodata::CheckerViaGeodata(TStringBuf geoDataFileName, int countryId) {
    const TAutoPtr<const NGeobase::TLookup> lookupPtr(new NGeobase::TLookup(static_cast<std::string>(geoDataFileName)));
    Y_ASSERT(lookupPtr.Get());

    const auto& idsList = lookupPtr->GetTree(countryId);
    for (const auto& id : idsList) {
        try {
            const auto& reg = lookupPtr->GetRegionById(id);
            if (0 < reg.GetType()) {
                TMappingTraits regTraits;
                {
                    regTraits.RegionId = reg.GetId();
                    regTraits.RegNamePath = reg.GetName().c_str();
                    regTraits.RegType = reg.GetType();
                    regTraits.Kind = TString(GetKind(reg.GetType()));
                }
                InsertItem(regTraits);
            }
        } catch (const std::exception& ex) {
            WARNING_LOG << "CheckerViaGeodata() ex: " << ex.what() << " // " << id << "\n";
        }
    }
    DEBUG_LOG << "CheckerViaGeodata(): found/inserted " << idsList.size() << "/" << Mapping.size() << " \n";
}

bool CheckerViaGeodata::IsNameOk(const TString& name) const {
    TStringBuf checkedName = name;
    {
        const TStringBuf sep = ", ";
        const size_t lastNamePos = checkedName.rfind(sep);
        if (name.npos != lastNamePos) {
            Y_ASSERT(lastNamePos + sep.size() < name.size());
            checkedName = checkedName.SubStr(lastNamePos + sep.size());
        }
    }
    return checkedName == GetMappingTraits().RegNamePath; // NB: 1) match Id, 2) type == Kind
}

bool CheckerViaGeodata::SearchName(TStringBuf /* name */) {
    return false;
}
