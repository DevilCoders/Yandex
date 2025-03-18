// Package geobase provides CGO bindings to the geobase library.
package geobase

import (
	"errors"
	"fmt"
	"time"
)

var ErrNotSupported = errors.New("geobase is not available when building with -DCGO_ENABLED=0")

const (
	RegionEarth   = 10000
	RegionMoscow  = 213
	RegionUnknown = 0
)

type ID int32

type IPFlags uint32

const (
	ipPlaceholder         = 1
	ipReserved            = 2
	ipYandexNet   IPFlags = 4
	ipYandexStaff IPFlags = 8
	ipYandexTurbo IPFlags = 16

	ipTor     IPFlags = 32
	ipProxy   IPFlags = 64
	ipVPN     IPFlags = 128
	ipHosting IPFlags = 256
	ipMobile  IPFlags = 512
)

func (ip IPFlags) IsEmpty() bool {
	return ip == 0
}

func (ip IPFlags) IsStub() bool {
	return ip&ipPlaceholder != 0
}

func (ip IPFlags) IsReserved() bool {
	return ip&ipReserved != 0
}

func (ip IPFlags) IsYandexNet() bool {
	return ip&ipYandexNet != 0
}

func (ip IPFlags) IsYandexStaff() bool {
	return ip&ipYandexStaff != 0
}

func (ip IPFlags) IsYandexTurbo() bool {
	return ip&ipYandexTurbo != 0
}

func (ip IPFlags) IsTor() bool {
	return ip&ipTor != 0
}

func (ip IPFlags) IsProxy() bool {
	return ip&ipProxy != 0
}

func (ip IPFlags) IsVPN() bool {
	return ip&ipVPN != 0
}

func (ip IPFlags) IsHosting() bool {
	return ip&ipHosting != 0
}

func (ip IPFlags) IsMobile() bool {
	return ip&ipMobile != 0
}

type RegionType int

const (
	RegionTypeRemoved            RegionType = -1
	RegionTypeOther              RegionType = 0
	RegionTypeContinent          RegionType = 1
	RegionTypeGeoRegion          RegionType = 2
	RegionTypeCountry            RegionType = 3
	RegionTypeCountryPart        RegionType = 4
	RegionTypeRegion             RegionType = 5
	RegionTypeCity               RegionType = 6
	RegionTypeVillage            RegionType = 7
	RegionTypeCityDistrict       RegionType = 8
	RegionTypeSubway             RegionType = 9
	RegionTypeDistrict           RegionType = 10
	RegionTypeAirport            RegionType = 11
	RegionTypeExtTerritory       RegionType = 12
	RegionTypeCityDistrictLevel2 RegionType = 13
	RegionTypeMonorail           RegionType = 14
	RegionTypeSettlement         RegionType = 15
)

type Error struct {
	Msg string
}

func (e *Error) Error() string {
	return fmt.Sprintf("geobase: %s", e.Msg)
}

type Region struct {
	ID, ParentID, CapitalID, CityID ID

	Type       RegionType
	Population int
	IsMain     bool
	Zoom       int
	Services   []string

	Name, EnName, ISOName, Synonyms string

	TimezoneName, ZipCode, PhoneCode string

	Latitude, Longitude         float64
	LatitudeSize, LongitudeSize float64

	// Dynamic stores all region fields.
	//
	// Possible field types are int32, uint32, bool, float64, string, []int32 and []string.
	//
	// Dynamic values of type []string are currently omitted.
	Dynamic map[string]interface{}
}

type CrimeaStatus int

const (
	CrimeaInUA    CrimeaStatus = 0
	CrimeaInRU    CrimeaStatus = 1
	CrimeaDefault              = CrimeaInRU
)

type Linguistics struct {
	NominativeCase    string
	GenitiveCase      string
	DativeCase        string
	PrepositionalCase string
	Preposition       string
	LocativeCase      string
	DirectionalCase   string
	AblativeCase      string
	AccusativeCase    string
	InstrumentalCase  string
}

type Timezone struct {
	Name, Abbr, Dst string
	Offset          time.Duration
}

type IPBasicTraits struct {
	RegionID ID
	Flags    IPFlags
}

type IPTraits struct {
	IPBasicTraits

	ISPName, ORGName, ASNList string
}

type GeolocationInput struct {
	YandexGID              int
	IP                     string
	GpAuto                 string
	XForwardedFor          string
	XRealIP                string
	UserPoints             string
	OverridePoint          string
	IsTrusted              bool
	AllowYandex            bool
	DisableSuspectedRegion bool
}

func NewGeolocationInput() GeolocationInput {
	// Following fields have default values in the corresponding C++-class.
	return GeolocationInput{
		YandexGID:   -1,
		IsTrusted:   true,
		AllowYandex: true,
	}
}

type GeoPrecision int

const (
	GeoPrecisionUnknown GeoPrecision = iota
	GeoPrecisionWifi
	GeoPrecisionCity
	GeoPrecisionRegion
	GeoPrecisionCountry
	GeoPrecisionTunefix
)

type GeoPoint struct {
	Lat float64
	Lon float64
}

type Geolocation struct {
	RegionID  int
	Precision GeoPrecision
	Location  GeoPoint
}

type Geobase interface {
	GetRegionByLocation(lat, lon float64, crimea ...CrimeaStatus) (*Region, error)
	GetRegionByIP(ip string, crimea ...CrimeaStatus) (*Region, error)
	GetRegionByID(id ID, crimea ...CrimeaStatus) (*Region, error)

	GetRegionIDByLocation(lat, lon float64) (ID, error)
	GetRegionIDByIP(ip string) (ID, error)

	GetChildrenIDs(id ID, crimea ...CrimeaStatus) ([]ID, error)
	GetParentsIDs(id ID, crimea ...CrimeaStatus) ([]ID, error)
	GetTreeIDs(id ID, crimea ...CrimeaStatus) ([]ID, error)
	GetCountryID(id ID, crimea ...CrimeaStatus) (ID, error)
	GetCapitalID(id ID) (ID, error)

	GetLinguistics(id ID, lang string) (*Linguistics, error)

	GetTimezoneByLocation(lat, lon float64) (*Timezone, error)
	GetTimezoneByID(id ID) (*Timezone, error)

	GetBasicTraitsByIP(ip string) (IPBasicTraits, error)
	GetTraitsByIP(ip string) (IPTraits, error)

	MakePinpointGeolocation(searchData GeolocationInput, ypCookie string, ysCookie string) (Geolocation, error)

	// Destroy underlying geobase wrapper freeing memory allocated by C++
	Destroy()
}
