package address_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/internal/address"
)

func TestParse(t *testing.T) {
	t.Run("positive cases", func(t *testing.T) {
		tests := []struct {
			addr string
			want address.Server
		}{
			{
				":5000",
				address.Server{Address: ":5000", Network: "tcp"},
			},
			{
				"[::1]:443",
				address.Server{Address: "[::1]:443", Network: "tcp"},
			},
			{
				"localhost:5000",
				address.Server{Address: "localhost:5000", Network: "tcp"},
			},
			{
				"tcp://localhost:42",
				address.Server{Address: "localhost:42", Network: "tcp"},
			},
			{
				"unix:///var/run/x.sock",
				address.Server{Address: "/var/run/x.sock", Network: "unix"},
			},
		}
		for _, tt := range tests {
			t.Run(tt.addr, func(t *testing.T) {
				got, err := address.Parse(tt.addr)
				require.NoError(t, err)
				require.Equal(t, tt.want, got)
			})
		}
	})

	t.Run("negative cases", func(t *testing.T) {
		for _, addr := range []string{
			"tcp://foo\tbar",
			"btc://foo.bar:42",
		} {
			t.Run(addr, func(t *testing.T) {
				_, err := address.Parse(addr)
				require.Error(t, err)
			})
		}

	})
}
