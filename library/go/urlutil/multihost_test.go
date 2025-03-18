package urlutil

import (
	"fmt"
	"net/url"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestNewMultihostURL(t *testing.T) {
	testCases := []struct {
		name        string
		input       *url.URL
		expected    *MultihostURL
		expectedErr error
	}{
		{
			"no_hosts",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				User:       url.UserPassword("username", "pass"),
				Host:       "",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			nil,
			ErrNoHosts,
		},
		{
			"single_host",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				User:       url.UserPassword("username", "pass"),
				Host:       "host1.sas.yc.yandex.net",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net",
				},
			},
			nil,
		},
		{
			"single_host_without_user",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				Host:       "host1.sas.yc.yandex.net",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					Host:       "host1.sas.yc.yandex.net",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net",
				},
			},
			nil,
		},
		{
			"single_host_with_port",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				User:       url.UserPassword("username", "pass"),
				Host:       "host1.sas.yc.yandex.net:5432",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net:5432",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net:5432",
				},
			},
			nil,
		},
		{
			"multiple_hosts_with_ports",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				User:       url.UserPassword("username", "pass"),
				Host:       "host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6543",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6543",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net:5432",
					"host2.man.yc.yandex.net:6543",
				},
			},
			nil,
		},
		{
			"multiple_hosts_with_default_port",
			&url.URL{
				Scheme:     "postgres",
				Opaque:     "",
				User:       url.UserPassword("username", "pass"),
				Host:       "host1.sas.yc.yandex.net,host2.man.yc.yandex.net:5432",
				Path:       "/mydb",
				RawPath:    "",
				ForceQuery: false,
				RawQuery:   "sslmode=verify-full",
				Fragment:   "",
			},
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net,host2.man.yc.yandex.net:5432",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net:5432",
					"host2.man.yc.yandex.net:5432",
				},
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			res, err := NewMultihostURL(tc.input)

			if tc.expectedErr != nil {
				assert.EqualError(t, err, tc.expectedErr.Error())
			} else {
				assert.NoError(t, err)
			}

			assert.Equal(t, tc.expected, res)

			if tc.input != nil && tc.input.User != nil && res != nil && res.User != nil {
				assert.NotEqual(t, fmt.Sprintf("%p", tc.input.User), fmt.Sprintf("%p", res.User))
			}
		})
	}
}

func TestNewMultihostURLFromString(t *testing.T) {
	testCases := []struct {
		name        string
		input       string
		expected    *MultihostURL
		expectedErr error
	}{
		{
			"no_hosts",
			"postgres://username:pass@/mydb?sslmode=verify-full",
			nil,
			ErrNoHosts,
		},
		{
			"single_host",
			"postgres://username:pass@host1.sas.yc.yandex.net/mydb?sslmode=verify-full",
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net",
				},
			},
			nil,
		},
		{
			"single_host_with_port",
			"postgres://username:pass@host1.sas.yc.yandex.net:5432/mydb?sslmode=verify-full",
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net:5432",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net:5432",
				},
			},
			nil,
		},
		{
			"single_host_without_user",
			"postgres://host1.sas.yc.yandex.net/mydb?sslmode=verify-full",
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					Host:       "host1.sas.yc.yandex.net",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net",
				},
			},
			nil,
		},
		{
			"multiple_hosts_with_ports",
			"postgres://username:pass@host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6543/mydb?sslmode=verify-full",
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net:5432,host2.man.yc.yandex.net:6543",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net:5432",
					"host2.man.yc.yandex.net:6543",
				},
			},
			nil,
		},
		{
			"multiple_hosts_with_default_ports",
			"postgres://username:pass@host1.sas.yc.yandex.net,host2.man.yc.yandex.net:5432/mydb?sslmode=verify-full",
			&MultihostURL{
				URL: &url.URL{
					Scheme:     "postgres",
					Opaque:     "",
					User:       url.UserPassword("username", "pass"),
					Host:       "host1.sas.yc.yandex.net,host2.man.yc.yandex.net:5432",
					Path:       "/mydb",
					RawPath:    "",
					ForceQuery: false,
					RawQuery:   "sslmode=verify-full",
					Fragment:   "",
				},
				Hosts: []string{
					"host1.sas.yc.yandex.net:5432",
					"host2.man.yc.yandex.net:5432",
				},
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			res, err := NewMultihostURLFromString(tc.input)

			if tc.expectedErr != nil {
				assert.EqualError(t, err, tc.expectedErr.Error())
			} else {
				assert.NoError(t, err)
			}

			assert.Equal(t, tc.expected, res)
		})
	}
}
