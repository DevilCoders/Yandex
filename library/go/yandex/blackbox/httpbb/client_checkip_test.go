package httpbb_test

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"net/url"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func TestClient_CheckIP(t *testing.T) {
	type TestCase struct {
		IP       string
		IsYandex bool
	}

	cases := []TestCase{
		{
			IP:       "1.1.1.1",
			IsYandex: false,
		},
		{
			IP:       "37.9.110.232",
			IsYandex: true,
		},
		{
			IP:       "2a02:6b8:b080:7308::1:7",
			IsYandex: true,
		},
	}

	tester := func(tc TestCase) {
		t.Run(tc.IP, func(t *testing.T) {
			t.Parallel()

			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				assert.Equal(t, "/blackbox", r.URL.Path)

				rawExpectedQuery := fmt.Sprintf("method=checkip&nets=yandexusers&format=json&ip=%s", url.QueryEscape(tc.IP))
				expectedQuery, err := url.ParseQuery(rawExpectedQuery)
				if assert.NoError(t, err) {
					assert.EqualValues(t, expectedQuery, r.URL.Query())
				}

				if tc.IsYandex {
					_, _ = w.Write([]byte(`{"yandexip":true}`))
				} else {
					_, _ = w.Write([]byte(`{"yandexip":false}`))
				}
			}))
			defer srv.Close()

			bbClient, err := httpbb.NewClient(httpbb.Environment{
				BlackboxHost: srv.URL,
			})
			require.NoError(t, err)

			response, err := bbClient.CheckIP(context.Background(), blackbox.CheckIPRequest{
				IP: tc.IP,
			})
			require.NoError(t, err)
			require.EqualValues(t, tc.IsYandex, response.YandexIP)
		})
	}

	for _, tc := range cases {
		tester(tc)
	}
}
