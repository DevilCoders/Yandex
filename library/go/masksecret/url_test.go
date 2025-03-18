package masksecret

import (
	"errors"
	"fmt"
	"math/rand"
	"net/url"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestURLString(t *testing.T) {
	testCases := []struct {
		name          string
		value         string
		expected      string
		expectedError error
	}{
		{
			"empty_url",
			"",
			"",
			nil,
		},
		{
			"invalid_url",
			"http_s://bad%url",
			"",
			errors.New("parse \"http_s://bad%url\": first path segment in URL cannot contain colon"),
		},
		{
			"no_sensitive_data",
			"https://user@localhost/data",
			"https://user@localhost/data",
			nil,
		},
		{
			"sensitive_data_in_passwd",
			"https://root:SUDOMAKEMEASANDWICH@localhost/data",
			"https://root:SxxxxxxxxxxxxxxxxxH@localhost/data",
			nil,
		},
		{
			"sensitive_data_in_passwd_is_placeholder",
			"https://root:xxxxxx@localhost/data",
			"https://xxxx:xxxxxx@localhost/data",
			nil,
		},
		{
			"sensitive_data_in_arg",
			"https://user@localhost/data?port=6543&password=SUDOMAKEMEASANDWICH",
			"https://user@localhost/data?password=SxxxxxxxxxxxxxxxxxH&port=6543",
			nil,
		},
		{
			"sensitive_data_in_args",
			"https://user@localhost/data?port=6543&password=SUDOMAKEMEASANDWICH&passwd=SUPERSUDO&passwd=SUPERSUDO",
			"https://user@localhost/data?passwd=SxxxxxxxO&passwd=SxxxxxxxO&password=SxxxxxxxxxxxxxxxxxH&port=6543",
			nil,
		},
		{
			"sensitive_data_in_passwd_and_args",
			"https://root:SUPERSUDO@localhost/data?port=6543&password=SUDOMAKEMEASANDWICH",
			"https://root:SxxxxxxxO@localhost/data?password=SxxxxxxxxxxxxxxxxxH&port=6543",
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			actual, err := URLString(tc.value)
			if tc.expectedError == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedError.Error())
			}
			assert.Equal(t, tc.expected, actual)
		})
	}
}

func TestURLCopy(t *testing.T) {
	testCases := []struct {
		name          string
		value         *url.URL
		expected      *url.URL
		expectedError error
	}{
		{
			"nil_url",
			nil,
			nil,
			errors.New("masksecret: nil pointer given"),
		},
		{
			"no_sensitive_data",
			&url.URL{
				Scheme: "https",
				User:   url.User("user"),
				Host:   "localhost",
				Path:   "/data",
			},
			&url.URL{
				Scheme: "https",
				User:   url.User("user"),
				Host:   "localhost",
				Path:   "/data",
			},
			nil,
		},
		{
			"sensitive_data_in_passwd",
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("root", "SUDOMAKEMEASANDWICH"),
				Host:   "localhost",
				Path:   "/data",
			},
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("root", "SxxxxxxxxxxxxxxxxxH"),
				Host:   "localhost",
				Path:   "/data",
			},
			nil,
		},
		{
			"sensitive_data_in_passwd_is_placeholder",
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("root", "xxxxx"),
				Host:   "localhost",
				Path:   "/data",
			},
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("xxxx", "xxxxx"),
				Host:   "localhost",
				Path:   "/data",
			},
			nil,
		},
		{
			"sensitive_data_in_arg",
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
			},
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "password=SxxxxxxxxxxxxxxxxxH&port=6543",
			},
			nil,
		},
		{
			"sensitive_data_in_args",
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH&passwd=SUPERSUDO&passwd=SUPERSUDO",
			},
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "passwd=SxxxxxxxO&passwd=SxxxxxxxO&password=SxxxxxxxxxxxxxxxxxH&port=6543",
			},
			nil,
		},
		{
			"sensitive_data_in_passwd_and_args",
			&url.URL{
				Scheme:   "https",
				User:     url.UserPassword("root", "SUPERSUDO"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
			},
			&url.URL{
				Scheme:   "https",
				User:     url.UserPassword("root", "SxxxxxxxO"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "password=SxxxxxxxxxxxxxxxxxH&port=6543",
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			actual, err := URLCopy(tc.value)
			if tc.expectedError == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedError.Error())
			}
			assert.Equal(t, tc.expected, actual)

			if actual != nil {
				assert.NotEqual(t, fmt.Sprintf("%p", tc.value), fmt.Sprintf("%p", actual))
			}
		})
	}
}

func TestURL(t *testing.T) {
	testCases := []struct {
		name          string
		value         *url.URL
		expected      *url.URL
		expectedError error
	}{
		{
			"nil_url",
			nil,
			nil,
			errors.New("masksecret: nil pointer given"),
		},
		{
			"no_sensitive_data",
			&url.URL{
				Scheme: "https",
				User:   url.User("user"),
				Host:   "localhost",
				Path:   "/data",
			},
			&url.URL{
				Scheme: "https",
				User:   url.User("user"),
				Host:   "localhost",
				Path:   "/data",
			},
			nil,
		},
		{
			"sensitive_data_in_passwd",
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("root", "SUDOMAKEMEASANDWICH"),
				Host:   "localhost",
				Path:   "/data",
			},
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("root", "SxxxxxxxxxxxxxxxxxH"),
				Host:   "localhost",
				Path:   "/data",
			},
			nil,
		},
		{
			"sensitive_data_in_passwd_is_placeholder",
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("root", "xxxxx"),
				Host:   "localhost",
				Path:   "/data",
			},
			&url.URL{
				Scheme: "https",
				User:   url.UserPassword("xxxx", "xxxxx"),
				Host:   "localhost",
				Path:   "/data",
			},
			nil,
		},
		{
			"sensitive_data_in_arg",
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
			},
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "password=SxxxxxxxxxxxxxxxxxH&port=6543",
			},
			nil,
		},
		{
			"sensitive_data_in_args",
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH&passwd=SUPERSUDO&passwd=SUPERSUDO",
			},
			&url.URL{
				Scheme:   "https",
				User:     url.User("user"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "passwd=SxxxxxxxO&passwd=SxxxxxxxO&password=SxxxxxxxxxxxxxxxxxH&port=6543",
			},
			nil,
		},
		{
			"sensitive_data_in_passwd_and_args",
			&url.URL{
				Scheme:   "https",
				User:     url.UserPassword("root", "SUPERSUDO"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
			},
			&url.URL{
				Scheme:   "https",
				User:     url.UserPassword("root", "SxxxxxxxO"),
				Host:     "localhost",
				Path:     "/data",
				RawQuery: "password=SxxxxxxxxxxxxxxxxxH&port=6543",
			},
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			err := URL(tc.value)
			if tc.expectedError == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, tc.expectedError, err.Error())
			}
			assert.Equal(t, tc.expected, tc.value)
		})
	}
}

func BenchmarkURLCopy(b *testing.B) {
	benchData := []*url.URL{
		{
			Scheme: "https",
			User:   url.User("user"),
			Host:   "localhost",
			Path:   "/data",
		},
		{
			Scheme: "https",
			User:   url.UserPassword("root", "SUDOMAKEMEASANDWICH"),
			Host:   "localhost",
			Path:   "/data",
		},
		{
			Scheme:   "https",
			User:     url.User("user"),
			Host:     "localhost",
			Path:     "/data",
			RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
		},
		{
			Scheme:   "https",
			User:     url.User("user"),
			Host:     "localhost",
			Path:     "/data",
			RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH&passwd=SUPERSUDO&passwd=SUPERSUDO",
		},
		{
			Scheme: "https",
			User:   url.User("user"),
			Host:   "localhost",
			Path:   "/data",
			RawQuery: url.Values{
				"port": {"6543"},
				"passwd": {
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
				},
			}.Encode(),
		},
		{
			Scheme:   "https",
			User:     url.UserPassword("user", "SUPERSUDO"),
			Host:     "localhost",
			Path:     "/data",
			RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
		},
	}

	r := rand.New(rand.NewSource(time.Now().UnixNano()))

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = URLCopy(benchData[r.Intn(len(benchData)-1)])
	}
}

func BenchmarkURL(b *testing.B) {
	benchData := []*url.URL{
		{
			Scheme: "https",
			User:   url.User("user"),
			Host:   "localhost",
			Path:   "/data",
		},
		{
			Scheme: "https",
			User:   url.UserPassword("root", "SUDOMAKEMEASANDWICH"),
			Host:   "localhost",
			Path:   "/data",
		},
		{
			Scheme:   "https",
			User:     url.User("user"),
			Host:     "localhost",
			Path:     "/data",
			RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
		},
		{
			Scheme:   "https",
			User:     url.User("user"),
			Host:     "localhost",
			Path:     "/data",
			RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH&passwd=SUPERSUDO&passwd=SUPERSUDO",
		},
		{
			Scheme: "https",
			User:   url.User("user"),
			Host:   "localhost",
			Path:   "/data",
			RawQuery: url.Values{
				"port": {"6543"},
				"passwd": {
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
					"SUDO", "SUDO", "SUDO", "SUDO", "SUDO",
				},
			}.Encode(),
		},
		{
			Scheme:   "https",
			User:     url.UserPassword("user", "SUPERSUDO"),
			Host:     "localhost",
			Path:     "/data",
			RawQuery: "port=6543&password=SUDOMAKEMEASANDWICH",
		},
	}

	r := rand.New(rand.NewSource(time.Now().UnixNano()))

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = URL(benchData[r.Intn(len(benchData)-1)])
	}
}
