package impl_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	fqdnlib "a.yandex-team.ru/cloud/mdb/internal/fqdn"
	fqdn "a.yandex-team.ru/cloud/mdb/internal/fqdn/impl"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestConverterManagedToUnmanaged(t *testing.T) {
	var inputs = []struct {
		In    string
		Out   string
		Error string
	}{
		{
			In:  "test.yandexcloud.net",
			Out: "test.yandexcloud.net",
		},
		{
			In:  "test.mdb.yandexcloud.net",
			Out: "test.mdb.yandexcloud.net",
		},
		{
			In:  "test.db.yandex.net",
			Out: "test.mdb.yandexcloud.net",
		},
		{
			In:    "test",
			Error: "no dot",
		},
		{
			In:    "test.",
			Error: "empty suffix",
		},
		{
			In:    "db.yandex.net",
			Error: "suffix \"yandex.net\" when supposed to be \"db.yandex.net\"",
		},
		{
			In:    "test.fake.db.yandex.net",
			Error: "suffix \"fake.db.yandex.net\" when supposed to be \"db.yandex.net\"",
		},
		{
			In:    "test.yandex.net",
			Error: "suffix \"yandex.net\" when supposed to be \"db.yandex.net\"",
		},
		{
			In:    "test.db.yandex",
			Error: "suffix \"db.yandex\" when supposed to be \"db.yandex.net\"",
		},
	}

	for _, input := range inputs {
		t.Run("convert "+input.In, func(t *testing.T) {
			converter := fqdn.NewConverter(
				"yandexcloud.net",
				"mdb.yandexcloud.net",
				"db.yandex.net",
			)
			res, err := converter.ManagedToUnmanaged(input.In)
			if len(input.Out) == 0 {
				assert.Error(t, err)
				assert.True(t, xerrors.Is(err, fqdnlib.ErrInvalidFQDN), "actual error: %s, required error: %s", err, input.Error)
				assert.Contains(t, err.Error(), input.Error)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, input.Out, res)
			}
		})
		t.Run("do not convert "+input.In, func(t *testing.T) {
			converter := fqdn.NewConverter("", "", "")
			res, err := converter.ManagedToUnmanaged(input.In)
			assert.NoError(t, err)
			assert.Equal(t, input.In, res)
		})
	}
}

func TestConverterUnmanagedToManaged(t *testing.T) {
	var inputs = []struct {
		In    string
		Out   string
		Error string
	}{
		{
			In:  "test.yandexcloud.net",
			Out: "test.yandexcloud.net",
		},
		{
			In:  "test.mdb.yandexcloud.net",
			Out: "test.db.yandex.net",
		},
		{
			In:  "test.db.yandex.net",
			Out: "test.db.yandex.net",
		},
		{
			In:    "test",
			Error: "no dot",
		},
		{
			In:    "test.",
			Error: "empty suffix",
		},
		{
			In:    "db.yandex.net",
			Error: "suffix \"yandex.net\" when supposed to be \"mdb.yandexcloud.net\"",
		},
		{
			In:    "test.fake.db.yandex.net",
			Error: "suffix \"fake.db.yandex.net\" when supposed to be \"mdb.yandexcloud.net\"",
		},
		{
			In:    "test.yandex.net",
			Error: "suffix \"yandex.net\" when supposed to be \"mdb.yandexcloud.net\"",
		},
		{
			In:    "test.db.yandex",
			Error: "suffix \"db.yandex\" when supposed to be \"mdb.yandexcloud.net\"",
		},
	}

	for _, input := range inputs {
		t.Run("convert "+input.In, func(t *testing.T) {
			converter := fqdn.NewConverter(
				"yandexcloud.net",
				"mdb.yandexcloud.net",
				"db.yandex.net",
			)
			res, err := converter.UnmanagedToManaged(input.In)
			if len(input.Out) == 0 {
				assert.Error(t, err)
				assert.True(t, xerrors.Is(err, fqdnlib.ErrInvalidFQDN), "actual error: %s, required error: %s", err, input.Error)
				assert.Contains(t, err.Error(), input.Error)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, input.Out, res)
			}
		})
		t.Run("do not convert "+input.In, func(t *testing.T) {
			converter := fqdn.NewConverter("", "", "")
			res, err := converter.UnmanagedToManaged(input.In)
			assert.NoError(t, err)
			assert.Equal(t, input.In, res)
		})
	}
}
