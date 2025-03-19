package net_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/x/net"
)

func TestIncrementPort(t *testing.T) {
	inputs := []struct {
		Name     string
		Addr     string
		Change   int16
		Expected string
		Error    bool
	}{
		{
			Name:  "Empty",
			Error: true,
		},
		{
			Name:     "Increment",
			Addr:     ":22",
			Change:   42,
			Expected: ":64",
		},
		{
			Name:     "Decrement",
			Addr:     ":22",
			Change:   -2,
			Expected: ":20",
		},
		{
			Name:     "v4Increment",
			Addr:     "127.0.0.1:22",
			Change:   42,
			Expected: "127.0.0.1:64",
		},
		{
			Name:     "v4Decrement",
			Addr:     "127.0.0.1:22",
			Change:   -2,
			Expected: "127.0.0.1:20",
		},
		{
			Name:   "v4NoPort",
			Addr:   "127.0.0.1",
			Change: 42,
			Error:  true,
		},
		{
			Name:   "v4Overflow",
			Addr:   "127.0.0.1:60000",
			Change: 5536,
			Error:  true,
		},
		{
			Name:   "v4Underflow",
			Addr:   "127.0.0.1:1",
			Change: -1,
			Error:  true,
		},
		{
			Name:     "v6Increment",
			Addr:     "[a:b:c:d::1:2]:22",
			Change:   42,
			Expected: "[a:b:c:d::1:2]:64",
		},
		{
			Name:     "v6Decrement",
			Addr:     "[a:b:c:d::1:2]:22",
			Change:   -2,
			Expected: "[a:b:c:d::1:2]:20",
		},
		{
			Name:   "v6NoPort",
			Addr:   "[a:b:c:d::1:2]",
			Change: 42,
			Error:  true,
		},
		{
			Name:   "v6Overflow",
			Addr:   "[a:b:c:d::1:2]:60000",
			Change: 5536,
			Error:  true,
		},
		{
			Name:   "v6Underflow",
			Addr:   "[a:b:c:d::1:2]:1",
			Change: -1,
			Error:  true,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			res, err := net.IncrementPort(input.Addr, input.Change)
			if input.Error {
				require.Error(t, err)
				require.Empty(t, res)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Expected, res)
		})
	}
}
