package optional_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func TestBool(t *testing.T) {
	opt := optional.Bool{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.False(t, ret)

	require.Panics(t, func() { opt.Must() })

	opt.Set(false)

	ret, err = opt.Get()
	require.NoError(t, err)
	require.False(t, ret)
	require.False(t, opt.Must())

	opt.Set(true)
	ret, err = opt.Get()
	require.NoError(t, err)
	require.True(t, ret)
}

func TestString(t *testing.T) {
	opt := optional.String{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.Equal(t, "", ret)

	require.Panics(t, func() { opt.Must() })

	opt.Set("WAT")

	ret, err = opt.Get()
	require.NoError(t, err)
	require.Equal(t, "WAT", ret)

	require.Equal(t, "WAT", opt.Must())
}

func TestInt64(t *testing.T) {
	opt := optional.Int64{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.Equal(t, int64(0), ret)

	require.Panics(t, func() { opt.Must() })

	opt.Set(42)

	ret, err = opt.Get()
	require.NoError(t, err)
	require.Equal(t, int64(42), ret)

	require.Equal(t, int64(42), opt.Must())

	tgt := optional.Int64{}
	err = tgt.UnmarshalJSON(nil)
	require.NoError(t, err)
	require.Equal(t, int64(0), tgt.Must())

	src := []byte("{\"Count\": 0, \"Int64\": 42, \"Valid\": true}")
	err = tgt.UnmarshalJSON(src)
	require.NoError(t, err)
	require.Equal(t, int64(42), tgt.Must())

	src, err = tgt.MarshalJSON()
	require.NoError(t, err)
	require.Equal(t, src, []byte("42"))

	tgt = optional.Int64{}
	src = []byte("42")
	err = tgt.UnmarshalJSON(src)
	require.NoError(t, err)
	require.Equal(t, int64(42), tgt.Must())

	src, err = tgt.MarshalJSON()
	require.NoError(t, err)
	require.Equal(t, src, []byte("42"))
}

func TestNewInt64(t *testing.T) {
	require.Equal(t, optional.Int64{Int64: int64(42), Valid: true}, optional.NewInt64(42))
}

func TestUint64(t *testing.T) {
	opt := optional.Uint64{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.Equal(t, uint64(0), ret)

	require.Panics(t, func() { opt.Must() })

	opt.Set(42)

	ret, err = opt.Get()
	require.NoError(t, err)
	require.Equal(t, uint64(42), ret)

	require.Equal(t, uint64(42), opt.Must())
}

func TestNewUint64(t *testing.T) {
	require.Equal(t, optional.Uint64{Uint64: uint64(42), Valid: true}, optional.NewUint64(42))
}

func TestFloat64(t *testing.T) {
	opt := optional.Float64{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.Equal(t, float64(0), ret)

	require.Panics(t, func() { opt.Must() })

	opt.Set(42.5)

	ret, err = opt.Get()
	require.NoError(t, err)
	require.Equal(t, float64(42.5), ret)

	require.Equal(t, float64(42.5), opt.Must())
}

func TestNewFloat64(t *testing.T) {
	require.Equal(t, optional.Float64{Float64: float64(42.6), Valid: true}, optional.NewFloat64(42.6))
}

func TestTime(t *testing.T) {
	opt := optional.Time{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.Equal(t, time.Time{}, ret)

	require.Panics(t, func() { opt.Must() })

	expected := time.Now()
	opt.Set(expected)

	ret, err = opt.Get()
	require.NoError(t, err)
	require.Equal(t, expected, ret)

	require.Equal(t, expected, opt.Must())
}

func TestNewTime(t *testing.T) {
	expected := time.Now()
	require.Equal(t, optional.Time{Time: expected, Valid: true}, optional.NewTime(expected))
}

func TestDuration(t *testing.T) {
	opt := optional.Duration{}

	ret, err := opt.Get()
	require.Error(t, err)
	require.Equal(t, time.Duration(0), ret)

	require.Panics(t, func() { opt.Must() })

	opt.Set(time.Hour)

	ret, err = opt.Get()
	require.NoError(t, err)
	require.Equal(t, time.Hour, ret)

	require.Equal(t, time.Hour, opt.Must())
}

func TestNewDuration(t *testing.T) {
	require.Equal(t, optional.Duration{Duration: time.Hour, Valid: true}, optional.NewDuration(time.Hour))
}
