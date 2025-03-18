%module internal

%{
#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/geobase/timezone_getter.hpp>
#include <geobase/include/service_getter.hpp>
%}

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

%template(IntList) std::vector<int>;
%template(StringList) std::vector<std::string>;

%inline %{
static void Swig_my_free(void* p) {
  free(p);
}
%}

typedef int64_t time_t;

%exception {
    try {
        $action;
    } catch (std::exception &e) {
        // This code leaks memory allocated by the local variables.
        // But we have no other choice, but to use panic() here.
        //
        // See https://github.com/swig/swig/issues/1131
        _swig_gopanic(e.what());
    }
}

namespace NGeobase {

struct TInitTraits {
    bool IsRefresh;
    bool IsLockMemory;
    bool IsPreloading;
    bool IsTzData;
    bool Id9KEnabled;
    std::string TimezonesPath;
    std::string DatafilePath;
};

namespace NImpl {

typedef int32_t Id;

namespace RT {

using UInt    = uint32_t;
using Int     = int32_t;
using Double  = double;
using Bool    = bool;
using String  = std::string;
using IntList = std::vector<int>;
using StrList = std::vector<String>;

enum FieldType {
    TYPE_UINT,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_BOOLEAN,
    TYPE_STRING,
    TYPE_INT_LIST,
};

} // namespace RT

class TServiceGetter {
public:
    using TBitmap = uint32_t;
    using TNamesList = std::vector<std::string>;

    TServiceGetter();

    const TNamesList& GetAllKnownNames() const;
    TNamesList GetNamesByMask(TBitmap mask) const;
};

using IdsList = RT::IntList;

struct TTimezone {
    std::string Name;
    std::string Abbr;
    std::string Dst;
    time_t Offset = 0;
};

struct TIpBasicTraits {
    uint32_t RegionId;
    uint32_t Flags;
};

struct TIpTraits : TIpBasicTraits {
    std::string IspName;
    std::string OrgName;
    std::string AsnList;
};

class TRegion {
public:
    Id GetId() const;
    Id GetParentId() const;
    Id GetCapitalId() const;
    Id GetCityId() const;

    RT::Int GetType() const;
    RT::UInt GetPopulation() const;
    RT::Bool IsMain() const;
    RT::Int GetZoom() const;
    RT::UInt GetServices() const;

    RT::String GetName() const;
    RT::String GetEnName() const;
    RT::String GetIsoName() const;
    RT::String GetSynonyms() const;

    std::string GetTimezoneName() const;
    std::string GetZipCode() const;
    std::string GetPhoneCode() const;

    RT::Double GetLatitude() const;
    RT::Double GetLongitude() const;
    RT::Double GetLatitudeSize() const;
    RT::Double GetLongitudeSize() const;

    RT::Bool GetBoolField(const std::string& fieldName) const;
    RT::Int GetIntField(const std::string& fieldName) const;
    RT::UInt GetUIntField(const std::string& fieldName) const;
    RT::Double GetDoubleField(const std::string& fieldName) const;
    const RT::String& GetStrField(const std::string& fieldName) const;
    const RT::IntList& GetIntListField(const std::string& fieldName) const;

    RT::StrList GetFieldsNames() const;
    RT::FieldType GetFieldType(const std::string& field_name) const;

private:
    TRegion();
};

struct TLinguistics {
    std::string NominativeCase;
    std::string GenitiveCase;
    std::string DativeCase;
    std::string PrepositionalCase;
    std::string Preposition;
    std::string LocativeCase;
    std::string DirectionalCase;
    std::string AblativeCase;
    std::string AccusativeCase;
    std::string InstrumentalCase;

    std::string lang;
};

struct TGeoPoint {
    static constexpr double OUT_THERE = 1000.;
    double Lat = OUT_THERE;
    double Lon = OUT_THERE;
};

template <typename StrType>
struct TGeolocationInputImpl {
    int32_t YandexGid = TGeolocation::ROOT_ID;
    StrType Ip;
    StrType GpAuto;
    StrType XForwardedFor;
    StrType XRealIp;
    StrType UserPoints;
    StrType OverridePoint;
    bool IsTrusted = true;
    bool AllowYandex = true;
    bool DisableSuspectedRegion = false;
};
using TNamesList = std::vector<std::string>;

struct TGeolocation {
    enum EGeoPrecision {
        GP_UNKNOWN,
        GP_WIFI,
        GP_CITY,
        GP_REGION,
        GP_COUNTRY,
        GP_TUNEFIX
    };
    int32_t RegionId          = ROOT_ID;
    int32_t Precision         = GP_UNKNOWN;
    TGeoPoint Location;
    static constexpr int ROOT_ID = -1;
};

%template(TGeolocationInput) TGeolocationInputImpl<std::string>;

class TLookup {
public:
    using TGeolocationInput = TGeolocationInputImpl<std::string>;
    enum CrimeaStatus {
        IN_UA,
        IN_RU
    };

    TLookup(const TInitTraits traits);

    TRegion GetRegionByIp(const std::string& ip, CrimeaStatus crimeaStatus) const;
    TRegion GetRegionById(Id id, CrimeaStatus crimeaStatus) const;

    TLinguistics GetLinguistics(Id id, const std::string& lang) const;

    Id GetRegionIdByIp(const std::string& ip) const;
    Id GetRegionIdByLocation(double latitude, double longitude) const;

    Id GetCapitalId(Id id) const;
    Id GetCountryId(Id id, CrimeaStatus crimeaStatus) const;

    TTimezone GetTimezoneByLocation(double latitude, double longitude) const;
    TTimezone GetTimezoneById(Id id) const;

    IdsList GetParentsIds(Id id, CrimeaStatus crimeaStatus) const;
    IdsList GetChildrenIds(Id id, CrimeaStatus crimeaStatus) const;
    IdsList GetTree(Id id, CrimeaStatus crimeaStatus) const;

    TIpBasicTraits GetBasicTraitsByIp(const std::string& ip) const;
    TIpTraits GetTraitsByIp(const std::string& ip) const;

    TGeolocation MakePinpointGeolocation(const TGeolocationInput& searchData, const std::string& ypCookie, const std::string& ysCookie) const;
};

} // namespace NImpl

}
