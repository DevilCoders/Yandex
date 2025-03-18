package masksecret

import (
	"fmt"
	"net/http"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/httputil/headers"
)

func TestHTTPRequest(t *testing.T) {
	testCases := []struct {
		name         string
		given        *http.Request
		extraHeaders []string
		expected     *http.Request
		expectErr    error
		expectSame   bool
	}{
		{
			"no_sensitive_data",
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				return r
			}(),
			nil,
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				return r
			}(),
			nil,
			true,
		},
		{
			"sensitive_query",
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url?password=ololo", nil)
				return r
			}(),
			nil,
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url?password=oxxxo", nil)
				return r
			}(),
			nil,
			false,
		},
		{
			"sensitive_headers",
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				r.Header.Set(headers.AuthorizationKey, "trololo")
				return r
			}(),
			nil,
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				r.Header.Set(headers.AuthorizationKey, "txxxxxo")
				return r
			}(),
			nil,
			false,
		},
		{
			"existent_cookies",
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				r.AddCookie(&http.Cookie{Name: "token", Value: "secret_token"})
				return r
			}(),
			nil,
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				r.AddCookie(&http.Cookie{Name: "token", Value: "sxxxxxxxxxxn"})
				return r
			}(),
			nil,
			false,
		},
		{
			"extra_headers",
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				r.Header.Set("X-Password", "shimba")
				r.Header.Set("X-Cache", "bypass")
				return r
			}(),
			[]string{"X-Password"},
			func() *http.Request {
				r, _ := http.NewRequest("GET", "/top/secure/url", nil)
				r.Header.Set("X-Password", "sxxxxa")
				r.Header.Set("X-Cache", "bypass")
				return r
			}(),
			nil,
			false,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			res, err := HTTPRequest(tc.given, tc.extraHeaders...)

			if tc.expectErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}

			assert.Equal(t, tc.expected, res)

			if tc.expectSame {
				assert.Same(t, tc.given, res)
			} else {
				assert.NotEqual(t, fmt.Sprintf("%p", tc.given), fmt.Sprintf("%p", res))
			}
		})
	}
}
