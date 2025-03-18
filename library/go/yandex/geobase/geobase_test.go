package geobase_test

import (
	"os"
	"testing"

	"github.com/davecgh/go-spew/spew"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/test/yatest"
	"a.yandex-team.ru/library/go/yandex/geobase"
)

func newGeobase(t testing.TB) geobase.Geobase {
	g, err := geobase.New(
		geobase.WithDatafilePath(yatest.WorkPath("geodata6.bin")),
		geobase.WithTimezonesPath(yatest.WorkPath("zones_bin")),
		geobase.WithTzData(true),
		geobase.WithPreload(true),
	)

	if err == geobase.ErrNotSupported {
		t.Skip("CGO_ENABLED=0 build is ok")
	}

	require.NoError(t, err)
	return g
}

func TestGeobase(t *testing.T) {
	g := newGeobase(t)
	defer g.Destroy()

	r, err := g.GetRegionByID(geobase.RegionMoscow)
	require.NoError(t, err)
	require.Equal(t, "Moscow", r.EnName)

	spew.Fdump(os.Stderr, r)

	_, err = g.GetRegionByID(91239412)
	require.Error(t, err)
	require.IsType(t, &geobase.Error{}, err)

	children, err := g.GetChildrenIDs(geobase.RegionMoscow)
	require.NoError(t, err)
	require.NotEmpty(t, children)

	parent, err := g.GetParentsIDs(geobase.RegionMoscow)
	require.NoError(t, err)
	require.NotEmpty(t, parent)
}

func TestListAllRegions(t *testing.T) {
	g := newGeobase(t)
	defer g.Destroy()

	allIDs, err := g.GetTreeIDs(geobase.RegionEarth)
	require.NoError(t, err)
	require.NotEmpty(t, allIDs)

	for _, id := range allIDs {
		_, err := g.GetRegionByID(id)
		require.NoError(t, err)

		_, err = g.GetLinguistics(id, "ru")
		require.NoError(t, err)

		_, err = g.GetTimezoneByID(id)
		require.NoError(t, err)

		_, err = g.GetCapitalID(id)
		require.NoError(t, err)

		_, err = g.GetCountryID(id)
		require.NoError(t, err)
	}
}

func TestIPTraits(t *testing.T) {
	g := newGeobase(t)
	defer g.Destroy()

	ip, err := g.GetTraitsByIP("77.88.8.8")
	require.NoError(t, err)
	require.NotEqual(t, ip.RegionID, geobase.RegionUnknown)

	basicIP, err := g.GetBasicTraitsByIP("77.88.8.8")
	require.NoError(t, err)
	require.NotEqual(t, basicIP.RegionID, geobase.RegionUnknown)
}

func BenchmarkGeobase(b *testing.B) {
	g := newGeobase(b)
	defer g.Destroy()

	b.ResetTimer()

	b.Run("GetRegionByID", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_, err := g.GetRegionByID(213)
			require.NoError(b, err)
		}
	})

	b.Run("GetRegionIDByLocation", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_, err := g.GetRegionIDByLocation(55.755768, 37.617671)
			require.NoError(b, err)
		}
	})
}

func TestMakePinpointGeolocation(t *testing.T) {
	g := newGeobase(t)
	defer g.Destroy()

	geoInput := geobase.NewGeolocationInput()
	geoInput.IP = "92.36.94.80"

	geolocation, err := g.MakePinpointGeolocation(geoInput, "", "")
	require.NoError(t, err)
	require.NotEqual(t, geolocation.RegionID, geobase.RegionUnknown)
	spew.Fdump(os.Stderr, geolocation)
}
