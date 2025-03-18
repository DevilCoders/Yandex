//go:build cgo
// +build cgo

package geobase

import (
	"fmt"
	"runtime"
	"time"

	"a.yandex-team.ru/library/go/cgosem"
	"a.yandex-team.ru/library/go/yandex/geobase/internal"
)

type cgoGeobase struct {
	c internal.TLookup
}

var _ Geobase = (*cgoGeobase)(nil)

func catch(err *error) {
	if p := recover(); p != nil {
		if s, ok := p.(string); ok {
			*err = &Error{Msg: s}
		} else {
			panic(p)
		}
	}
}

type Option func(t internal.TInitTraits)

func (c CrimeaStatus) cgo() internal.NGeobaseNImplTLookupCrimeaStatus {
	return internal.NGeobaseNImplTLookupCrimeaStatus(c)
}

func WithRefresh(b bool) Option {
	return func(t internal.TInitTraits) {
		t.SetIsRefresh(b)
	}
}

func WithLockMemory(b bool) Option {
	return func(t internal.TInitTraits) {
		t.SetIsLockMemory(b)
	}
}

func WithPreload(b bool) Option {
	return func(t internal.TInitTraits) {
		t.SetIsPreloading(b)
	}
}

func WithTzData(b bool) Option {
	return func(t internal.TInitTraits) {
		t.SetIsTzData(b)
	}
}

func WithID9KEnabled(b bool) Option {
	return func(t internal.TInitTraits) {
		t.SetId9KEnabled(b)
	}
}

func WithDatafilePath(path string) Option {
	return func(t internal.TInitTraits) {
		t.SetDatafilePath(path)
	}
}

// WithTimezonePath specifies path to the directory containing timezone database.
//
// path MUST be absolute.
func WithTimezonesPath(path string) Option {
	return func(t internal.TInitTraits) {
		t.SetTimezonesPath(path)
	}
}

// New creates new geobase object.
//
// When building with cgo disabled, New is replaced by the stub returning ErrNotSupported.
func New(opts ...Option) (g Geobase, err error) {
	defer catch(&err)

	o := internal.NewTInitTraits()
	defer internal.DeleteTInitTraits(o)

	for _, opt := range opts {
		opt(o)
	}

	g = &cgoGeobase{c: internal.NewTLookup(o)}
	runtime.SetFinalizer(g, (*cgoGeobase).Destroy)
	return
}

func (g *cgoGeobase) Destroy() {
	if g.c != nil {
		internal.DeleteTLookup(g.c)
		g.c = nil
		runtime.SetFinalizer(g, nil)
	}
}

func unpackRegion(r internal.TRegion) *Region {
	dynamic := map[string]interface{}{}

	names := r.GetFieldsNames()
	defer internal.DeleteStringList(names)

	serviceGetter := internal.NewTServiceGetter()
	defer internal.DeleteTServiceGetter(serviceGetter)

	servicesList := serviceGetter.GetNamesByMask(r.GetServices())
	defer internal.DeleteStringList(servicesList)

	var services []string
	for i := 0; i < int(servicesList.Size()); i++ {
		services = append(services, servicesList.Get(i))
	}

	for i := 0; i < int(names.Size()); i++ {
		name := names.Get(i)

		if name == "services" {
			dynamic[name] = services
		}

		switch typ := r.GetFieldType(name); typ {
		case internal.TYPE_BOOLEAN:
			dynamic[name] = r.GetBoolField(name)
		case internal.TYPE_DOUBLE:
			dynamic[name] = r.GetDoubleField(name)
		case internal.TYPE_STRING:
			dynamic[name] = r.GetStrField(name)
		case internal.TYPE_INT:
			dynamic[name] = int32(r.GetIntField(name))
		case internal.TYPE_UINT:
			dynamic[name] = uint32(r.GetUIntField(name))

		case internal.TYPE_INT_LIST:
			// This type is used by suggest_list and linguistics_list. Both are not interesting for the user.

		default:
			panic(fmt.Sprintf("unsupported type %d in field %s", typ, name))
		}
	}

	return &Region{
		ID:        ID(r.GetId()),
		ParentID:  ID(r.GetParentId()),
		CapitalID: ID(r.GetCapitalId()),
		CityID:    ID(r.GetCityId()),

		Type:       RegionType(r.GetType()),
		Population: int(r.GetPopulation()),
		IsMain:     r.IsMain(),
		Zoom:       r.GetZoom(),
		Services:   services,

		Name:     r.GetName(),
		EnName:   r.GetEnName(),
		ISOName:  r.GetIsoName(),
		Synonyms: r.GetSynonyms(),

		TimezoneName: r.GetTimezoneName(),
		ZipCode:      r.GetZipCode(),
		PhoneCode:    r.GetPhoneCode(),

		Latitude:      r.GetLatitude(),
		Longitude:     r.GetLongitude(),
		LatitudeSize:  r.GetLatitudeSize(),
		LongitudeSize: r.GetLongitudeSize(),

		Dynamic: dynamic,
	}
}

func unpackLinguistics(l internal.TLinguistics) *Linguistics {
	return &Linguistics{
		NominativeCase:    l.GetNominativeCase(),
		DativeCase:        l.GetDativeCase(),
		DirectionalCase:   l.GetDirectionalCase(),
		GenitiveCase:      l.GetGenitiveCase(),
		LocativeCase:      l.GetLocativeCase(),
		Preposition:       l.GetPreposition(),
		PrepositionalCase: l.GetPrepositionalCase(),
		AblativeCase:      l.GetAblativeCase(),
		AccusativeCase:    l.GetAccusativeCase(),
		InstrumentalCase:  l.GetInstrumentalCase(),
	}
}

func unpackTimezone(l internal.TTimezone) *Timezone {
	return &Timezone{
		Name:   l.GetName(),
		Abbr:   l.GetAbbr(),
		Dst:    l.GetDst(),
		Offset: time.Second * time.Duration(l.GetOffset()),
	}
}

func crimeaStatus(crimea ...CrimeaStatus) internal.NGeobaseNImplTLookupCrimeaStatus {
	if len(crimea) == 0 {
		return internal.TLookupIN_RU
	}

	return crimea[0].cgo()
}

func (g *cgoGeobase) GetRegionByLocation(lat, lon float64, crimea ...CrimeaStatus) (r *Region, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	id := g.c.GetRegionIdByLocation(lat, lon)

	rr := g.c.GetRegionById(id, crimeaStatus(crimea...))
	defer internal.DeleteTRegion(rr)

	r = unpackRegion(rr)
	return
}

func (g *cgoGeobase) GetRegionByIP(ip string, crimea ...CrimeaStatus) (r *Region, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	rr := g.c.GetRegionByIp(ip, crimeaStatus(crimea...))
	defer internal.DeleteTRegion(rr)

	r = unpackRegion(rr)
	return
}

func (g *cgoGeobase) GetRegionByID(id ID, crimea ...CrimeaStatus) (r *Region, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	rr := g.c.GetRegionById(int(id), crimeaStatus(crimea...))
	defer internal.DeleteTRegion(rr)

	r = unpackRegion(rr)
	return
}

func (g *cgoGeobase) GetRegionIDByLocation(lat, lon float64) (id ID, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	id = ID(g.c.GetRegionIdByLocation(lat, lon))
	return
}

func (g *cgoGeobase) GetRegionIDByIP(ip string) (id ID, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	id = ID(g.c.GetRegionIdByIp(ip))
	return
}

func (g *cgoGeobase) GetCountryID(id ID, crimea ...CrimeaStatus) (country ID, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	country = ID(g.c.GetCountryId(int(id), crimeaStatus(crimea...)))
	return
}

func (g *cgoGeobase) GetCapitalID(id ID) (capital ID, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	capital = ID(g.c.GetCapitalId(int(id)))
	return
}

func (g *cgoGeobase) getIDList(
	id ID, crimea []CrimeaStatus,
	method func(int, internal.NGeobaseNImplTLookupCrimeaStatus) internal.IntList,
) (ids []ID, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	l := method(int(id), crimeaStatus(crimea...))
	defer internal.DeleteIntList(l)

	for i := 0; i < int(l.Size()); i++ {
		ids = append(ids, ID(l.Get(i)))
	}
	return
}

func (g *cgoGeobase) GetChildrenIDs(id ID, crimea ...CrimeaStatus) (ids []ID, err error) {
	return g.getIDList(id, crimea, g.c.GetChildrenIds)
}

func (g *cgoGeobase) GetParentsIDs(id ID, crimea ...CrimeaStatus) (ids []ID, err error) {
	return g.getIDList(id, crimea, g.c.GetParentsIds)
}

func (g *cgoGeobase) GetTreeIDs(id ID, crimea ...CrimeaStatus) (ids []ID, err error) {
	return g.getIDList(id, crimea, g.c.GetTree)
}

func (g *cgoGeobase) GetLinguistics(id ID, lang string) (l *Linguistics, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	ll := g.c.GetLinguistics(int(id), lang)
	defer internal.DeleteTLinguistics(ll)

	l = unpackLinguistics(ll)
	return
}

func (g *cgoGeobase) GetTimezoneByLocation(lat, lon float64) (tz *Timezone, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	ctz := g.c.GetTimezoneByLocation(lat, lon)
	defer internal.DeleteTTimezone(ctz)

	tz = unpackTimezone(ctz)
	return
}

func (g *cgoGeobase) GetTimezoneByID(id ID) (tz *Timezone, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	ctz := g.c.GetTimezoneById(int(id))
	defer internal.DeleteTTimezone(ctz)

	tz = unpackTimezone(ctz)
	return
}

func unpackBasicIPTraits(tt internal.TIpBasicTraits) IPBasicTraits {
	return IPBasicTraits{
		RegionID: ID(tt.GetRegionId()),
		Flags:    IPFlags(tt.GetFlags()),
	}
}

func (g *cgoGeobase) GetBasicTraitsByIP(ip string) (t IPBasicTraits, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	tt := g.c.GetBasicTraitsByIp(ip)
	defer internal.DeleteTIpBasicTraits(tt)

	t = unpackBasicIPTraits(tt)
	return
}

func unpackIPTraits(tt internal.TIpTraits) IPTraits {
	return IPTraits{
		IPBasicTraits: IPBasicTraits{
			RegionID: ID(tt.GetRegionId()),
			Flags:    IPFlags(tt.GetFlags()),
		},

		ISPName: tt.GetIspName(),
		ORGName: tt.GetOrgName(),
		ASNList: tt.GetAsnList(),
	}
}

func (g *cgoGeobase) GetTraitsByIP(ip string) (t IPTraits, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	tt := g.c.GetTraitsByIp(ip)
	defer internal.DeleteTIpTraits(tt)

	t = unpackIPTraits(tt)
	return
}

func packGeolocationInput(location GeolocationInput) internal.TGeolocationInput {
	internalGeolocation := internal.NewTGeolocationInput()
	internalGeolocation.SetYandexGid(location.YandexGID)
	internalGeolocation.SetIp(location.IP)
	internalGeolocation.SetGpAuto(location.GpAuto)
	internalGeolocation.SetXForwardedFor(location.XForwardedFor)
	internalGeolocation.SetXRealIp(location.XRealIP)
	internalGeolocation.SetUserPoints(location.UserPoints)
	internalGeolocation.SetOverridePoint(location.OverridePoint)
	internalGeolocation.SetIsTrusted(location.IsTrusted)
	internalGeolocation.SetAllowYandex(location.AllowYandex)
	internalGeolocation.SetDisableSuspectedRegion(location.DisableSuspectedRegion)
	return internalGeolocation
}

func unpackGeolocation(geolocation internal.TGeolocation) Geolocation {
	return Geolocation{
		RegionID:  geolocation.GetRegionId(),
		Precision: GeoPrecision(geolocation.GetPrecision()),
		Location: GeoPoint{
			Lat: geolocation.GetLocation().GetLat(),
			Lon: geolocation.GetLocation().GetLon(),
		},
	}
}

func (g *cgoGeobase) MakePinpointGeolocation(searchData GeolocationInput, ypCookie string, ysCookie string) (location Geolocation, err error) {
	defer cgosem.S.Acquire().Release()
	defer catch(&err)

	internalSearchData := packGeolocationInput(searchData)
	defer internal.DeleteTGeolocationInput(internalSearchData)
	geolocation := g.c.MakePinpointGeolocation(internalSearchData, ypCookie, ysCookie)
	defer internal.DeleteTGeolocation(geolocation)

	location = unpackGeolocation(geolocation)
	return
}
