package httplaas_test

import (
	"context"
	"errors"
	"net"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/yandex/laas"
	"a.yandex-team.ru/library/go/yandex/laas/httplaas"
)

func TestClient_DetectRegion(t *testing.T) {
	testCases := []struct {
		name           string
		bootstrap      func() (*httptest.Server, *httplaas.Client)
		params         laas.Params
		expectResponse *laas.RegionResponse
		expectError    error
	}{
		{
			"net_error",
			func() (*httptest.Server, *httplaas.Client) {
				var ts *httptest.Server
				ts = httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					ts.CloseClientConnections()
				}))

				c, _ := httplaas.NewClient(
					httplaas.AsService("arcadia"),
					httplaas.WithAppUUID(uuid.Must(uuid.FromString("1d1cee6e-e07a-4a90-ab31-2545d59049e2"))),
					httplaas.WithHTTPHost(ts.URL),
				)
				return ts, c
			},
			laas.Params{
				UserIP:    net.IP{192, 168, 0, 42},
				YandexUID: 5234123166213,
				YandexGID: 213,
				YP:        "1503399579.gpauto.55_743237:37_605911:150:1:1503392379#1531388667.ygu.0",
				URLPrefix: "http://distbuild.yandex-team.ru/laas-test",
			},
			nil,
			errors.New("EOF"),
		},
		{
			"bad_status_code",
			func() (*httptest.Server, *httplaas.Client) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					w.WriteHeader(http.StatusBadRequest)
				}))

				c, _ := httplaas.NewClient(
					httplaas.AsService("arcadia"),
					httplaas.WithAppUUID(uuid.Must(uuid.FromString("1d1cee6e-e07a-4a90-ab31-2545d59049e2"))),
					httplaas.WithHTTPHost(ts.URL),
				)
				return ts, c
			},
			laas.Params{
				UserIP:    net.IP{192, 168, 0, 42},
				YandexUID: 5234123166213,
				YandexGID: 213,
				YP:        "1503399579.gpauto.55_743237:37_605911:150:1:1503392379#1531388667.ygu.0",
				URLPrefix: "http://distbuild.yandex-team.ru/laas-test",
			},
			nil,
			errors.New("laas: bad HTTP status code 400"),
		},
		{
			"bad_body",
			func() (*httptest.Server, *httplaas.Client) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte("olololo"))
				}))

				c, _ := httplaas.NewClient(
					httplaas.AsService("arcadia"),
					httplaas.WithAppUUID(uuid.Must(uuid.FromString("1d1cee6e-e07a-4a90-ab31-2545d59049e2"))),
					httplaas.WithHTTPHost(ts.URL),
				)
				return ts, c
			},
			laas.Params{
				UserIP:    net.IP{192, 168, 0, 42},
				YandexUID: 5234123166213,
				YandexGID: 213,
				YP:        "1503399579.gpauto.55_743237:37_605911:150:1:1503392379#1531388667.ygu.0",
				URLPrefix: "http://distbuild.yandex-team.ru/laas-test",
			},
			nil,
			errors.New("invalid character 'o' looking for beginning of value"),
		},
		{
			"success",
			func() (*httptest.Server, *httplaas.Client) {
				ts := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
					b := `
						{
						  "region_id": 121779,
						  "precision": 2,
						  "latitude": 54.295447,
						  "longitude": 37.570887,
						  "should_update_cookie": true,
						  "is_user_choice": false,
						  "suspected_region_id": 121779,
						  "city_id": 121779,
						  "region_by_ip": 121779,
						  "suspected_region_city": 121779,
						  "location_accuracy": 15000,
						  "location_unixtime": 1560765105,
						  "suspected_latitude": 54.295447,
						  "suspected_longitude": 37.570887,
						  "suspected_location_accuracy": 15000,
						  "suspected_location_unixtime": 1560765105,
						  "suspected_precision": 2,
						  "region_home": 121779,
						  "probable_regions_reliability": 1.00,
						  "probable_regions": [],
						  "country_id_by_ip": 225,
						  "is_anonymous_vpn": false,
						  "is_public_proxy": false,
						  "is_serp_trusted_net": false,
						  "is_tor": false,
						  "is_hosting": false,
						  "is_gdpr": false,
						  "is_mobile": false,
						  "is_yandex_net": false,
						  "is_yandex_staff": false
						}
					`
					_, _ = w.Write([]byte(b))
				}))

				c, _ := httplaas.NewClient(
					httplaas.AsService("arcadia"),
					httplaas.WithAppUUID(uuid.Must(uuid.FromString("1d1cee6e-e07a-4a90-ab31-2545d59049e2"))),
					httplaas.WithHTTPHost(ts.URL),
				)
				return ts, c
			},
			laas.Params{
				UserIP:    net.IP{192, 168, 0, 42},
				YandexUID: 5234123166213,
				YandexGID: 213,
				YP:        "1503399579.gpauto.55_743237:37_605911:150:1:1503392379#1531388667.ygu.0",
				URLPrefix: "http://distbuild.yandex-team.ru/laas-test",
			},
			&laas.RegionResponse{
				RegionID:                   121779,
				Precision:                  2,
				Latitude:                   54.295447,
				Longitude:                  37.570887,
				ShouldUpdateCookie:         true,
				IsUserChoice:               false,
				SuspectedRegionID:          121779,
				CityID:                     121779,
				RegionByIP:                 121779,
				SuspectedRegionCity:        121779,
				LocationAccuracy:           15000,
				LocationUnixtime:           laas.Timestamp{Time: time.Unix(1560765105, 0)},
				SuspectedLatitude:          54.295447,
				SuspectedLongitude:         37.570887,
				SuspectedLocationAccuracy:  15000,
				SuspectedLocationUnixtime:  laas.Timestamp{Time: time.Unix(1560765105, 0)},
				SuspectedPrecision:         2,
				ProbableRegionsReliability: 1,
				ProbableRegions:            []laas.ProbableRegion{},
				CountryIDByIP:              225,
				IsAnonymousVpn:             false,
				IsPublicProxy:              false,
				IsSerpTrustedNet:           false,
				IsTor:                      false,
				IsHosting:                  false,
				IsGdpr:                     false,
				IsMobile:                   false,
				IsYandexNet:                false,
				IsYandexStaff:              false,
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ts, c := tc.bootstrap()
			defer ts.Close()

			ctx := context.Background()
			resp, err := c.DetectRegion(ctx, tc.params)

			if tc.expectError == nil {
				assert.NoError(t, err)
			} else {
				assert.True(t,
					err != nil &&
						(err == tc.expectError ||
							err.Error() == tc.expectError.Error() ||
							strings.Contains(err.Error(), tc.expectError.Error())),
					"expecting error: %+v, got: %+v", tc.expectError, err,
				)
			}

			assert.Equal(t, tc.expectResponse, resp)
		})
	}
}
