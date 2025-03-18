package urlutil

import (
	"net/url"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestMustParse(t *testing.T) {
	testCases := []struct {
		input       string
		expectedURL *url.URL
		expectPanic bool
	}{
		{
			"ololo\r\ntrololo",
			nil,
			true,
		},
		{
			"postgres://username:pass@host1.sas.yc.yandex.net:6432/mydb?sslmode=verify-full",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				User:       url.UserPassword("username", "pass"),
				Host:       "host1.sas.yc.yandex.net:6432",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			false,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.input, func(t *testing.T) {
			if tc.expectPanic {
				var res *url.URL
				assert.Panics(t, func() { res = MustParse(tc.input) })
				assert.Equal(t, tc.expectedURL, res)
			} else {
				assert.Equal(t, tc.expectedURL, MustParse(tc.input))
			}
		})
	}
}
