package intervals

import (
	"math"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestBoundInt64(t *testing.T) {
	t.Run("Inclusive", func(t *testing.T) {
		b := BoundInt64{Type: Inclusive}
		assert.True(t, b.inMinBound(math.MaxInt64))
		assert.True(t, b.inMinBound(1))
		assert.True(t, b.inMinBound(0))
		assert.False(t, b.inMinBound(-1))
		assert.False(t, b.inMinBound(math.MinInt64))

		assert.True(t, b.inMaxBound(math.MinInt64))
		assert.True(t, b.inMaxBound(-1))
		assert.True(t, b.inMaxBound(0))
		assert.False(t, b.inMaxBound(1))
		assert.False(t, b.inMaxBound(math.MaxInt64))
	})

	t.Run("Exclusive", func(t *testing.T) {
		b := BoundInt64{Type: Exclusive}
		assert.True(t, b.inMinBound(math.MaxInt64))
		assert.True(t, b.inMinBound(1))
		assert.False(t, b.inMinBound(0))
		assert.False(t, b.inMinBound(-1))
		assert.False(t, b.inMinBound(math.MinInt64))

		assert.True(t, b.inMaxBound(math.MinInt64))
		assert.True(t, b.inMaxBound(-1))
		assert.False(t, b.inMaxBound(0))
		assert.False(t, b.inMaxBound(1))
		assert.False(t, b.inMaxBound(math.MaxInt64))
	})

	t.Run("Unbounded", func(t *testing.T) {
		b := BoundInt64{Type: Unbounded}
		assert.True(t, b.inMinBound(math.MaxInt64))
		assert.True(t, b.inMinBound(1))
		assert.True(t, b.inMinBound(0))
		assert.True(t, b.inMinBound(-1))
		assert.True(t, b.inMinBound(math.MinInt64))

		assert.True(t, b.inMaxBound(math.MinInt64))
		assert.True(t, b.inMaxBound(-1))
		assert.True(t, b.inMaxBound(0))
		assert.True(t, b.inMaxBound(1))
		assert.True(t, b.inMaxBound(math.MaxInt64))
	})
}

func TestInt64_New(t *testing.T) {
	inputs := []struct {
		Name    string
		Min     int64
		MinType BoundType
		Max     int64
		MaxType BoundType
		Invalid bool
	}{
		{
			Name:    "UnboundedMin",
			MinType: Unbounded,
			Max:     math.MinInt64,
			MaxType: Exclusive,
		},
		{
			Name:    "UnboundedMax",
			Min:     math.MaxInt64,
			MinType: Exclusive,
			MaxType: Unbounded,
		},
		{
			Name:    "Exclusive negative diff",
			Min:     2,
			MinType: Exclusive,
			Max:     1,
			MaxType: Exclusive,
			Invalid: true,
		},
		{
			Name:    "Inclusive negative diff",
			Min:     2,
			MinType: Inclusive,
			Max:     1,
			MaxType: Inclusive,
			Invalid: true,
		},
		{
			Name:    "Exclusive diff < 2",
			Min:     1,
			MinType: Exclusive,
			Max:     2,
			MaxType: Exclusive,
			Invalid: true,
		},
		{
			Name:    "Exclusive diff >= 2",
			Min:     1,
			MinType: Exclusive,
			Max:     3,
			MaxType: Exclusive,
		},
		{
			Name:    "InclusiveExclusive diff < 1",
			Min:     1,
			MinType: Inclusive,
			Max:     1,
			MaxType: Exclusive,
			Invalid: true,
		},
		{
			Name:    "ExclusiveInclusive diff < 1",
			Min:     1,
			MinType: Exclusive,
			Max:     1,
			MaxType: Inclusive,
			Invalid: true,
		},
		{
			Name:    "InclusiveExclusive diff >= 1",
			Min:     1,
			MinType: Inclusive,
			Max:     2,
			MaxType: Exclusive,
		},
		{
			Name:    "ExclusiveInclusive diff >= 1",
			Min:     1,
			MinType: Exclusive,
			Max:     2,
			MaxType: Inclusive,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			_, err := NewInt64(input.Min, input.MinType, input.Max, input.MaxType)
			if input.Invalid {
				require.Error(t, err)
			} else {
				require.NoError(t, err)
			}
		})
	}
}

func TestInt64_Includes(t *testing.T) {
	inputs := []struct {
		Name     string
		Interval Int64
		Value    int64
		Res      bool
	}{
		{
			Name:     "UnboundedMin",
			Interval: MustNewInt64(0, Unbounded, 0, Inclusive),
			Value:    math.MinInt64,
			Res:      true,
		},
		{
			Name:     "UnboundedMax",
			Interval: MustNewInt64(0, Inclusive, 0, Unbounded),
			Value:    math.MaxInt64,
			Res:      true,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			require.Equal(t, input.Res, input.Interval.Includes(input.Value))
		})
	}
}

func TestInt64_String(t *testing.T) {
	inputs := []struct {
		Name     string
		Interval Int64
		String   string
	}{
		{
			Name:   "Default",
			String: "?0,0?",
		},
		{
			Name:     "UnboundedInclusive",
			Interval: MustNewInt64(0, Unbounded, 0, Inclusive),
			String:   "(,0]",
		},
		{
			Name:     "InclusiveUnbounded",
			Interval: MustNewInt64(0, Inclusive, 0, Unbounded),
			String:   "[0,)",
		},
		{
			Name:     "InclusiveExclusive",
			Interval: MustNewInt64(-1, Inclusive, 1, Exclusive),
			String:   "[-1,1)",
		},
		{
			Name:     "Inclusive",
			Interval: MustNewInt64(-1, Inclusive, 1, Inclusive),
			String:   "[-1,1]",
		},
		{
			Name:     "Exclusive",
			Interval: MustNewInt64(-1, Exclusive, 1, Exclusive),
			String:   "(-1,1)",
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			require.Equal(t, input.String, input.Interval.String())
		})
	}
}
