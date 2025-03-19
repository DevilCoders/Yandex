package quota

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func TestResources_Add(t *testing.T) {
	base := Resources{
		CPU:      1,
		GPU:      1,
		Memory:   1024,
		SSDSpace: 1024,
		HDDSpace: 1024,
		Clusters: 1,
	}

	add := Resources{
		CPU:      1,
		GPU:      1,
		Memory:   1024,
		SSDSpace: 1024,
		HDDSpace: 1024,
		Clusters: 1,
	}

	expected := Resources{
		CPU:      2,
		GPU:      2,
		Memory:   2048,
		SSDSpace: 2048,
		HDDSpace: 2048,
		Clusters: 2,
	}

	require.Equal(t, expected, base.Add(add))
}

func TestResources_Sub(t *testing.T) {
	base := Resources{
		CPU:      2,
		GPU:      2,
		Memory:   2048,
		SSDSpace: 2048,
		HDDSpace: 2048,
		Clusters: 2,
	}

	sub := Resources{
		CPU:      1,
		GPU:      1,
		Memory:   1024,
		SSDSpace: 1024,
		HDDSpace: 1024,
		Clusters: 1,
	}

	expected := Resources{
		CPU:      1,
		GPU:      1,
		Memory:   1024,
		SSDSpace: 1024,
		HDDSpace: 1024,
		Clusters: 1,
	}

	require.Equal(t, expected, base.Sub(sub))
}

func TestResources_Mul(t *testing.T) {
	t.Run("Empty", func(t *testing.T) {
		require.Equal(t, Resources{}, Resources{}.Mul(1))
	})

	t.Run("Zero", func(t *testing.T) {
		require.Equal(t, Resources{}, Resources{CPU: 1}.Mul(0))
	})

	t.Run("One", func(t *testing.T) {
		r := Resources{
			CPU:      1,
			GPU:      1,
			Memory:   1024,
			SSDSpace: 1024,
			HDDSpace: 1024,
			Clusters: 1,
		}
		require.Equal(t, r, r.Mul(1))
	})

	t.Run("Five", func(t *testing.T) {
		r := Resources{
			CPU:      1,
			GPU:      1,
			Memory:   1024,
			SSDSpace: 1024,
			HDDSpace: 1024,
			Clusters: 1,
		}
		expected := Resources{
			CPU:      5,
			GPU:      5,
			Memory:   5 * 1024,
			SSDSpace: 5 * 1024,
			HDDSpace: 5 * 1024,
			Clusters: 5,
		}
		require.Equal(t, expected, r.Mul(5))
	})
}

func TestConsumption_RequestChange(t *testing.T) {
	t.Run("Empty", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		c.RequestChange(Resources{})
		require.Equal(t, Resources{}, c.quota)
		require.Equal(t, Resources{}, c.used)
		require.Equal(t, Resources{}, c.diff)
	})

	t.Run("Add", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		r := Resources{
			CPU:      1,
			GPU:      1,
			Memory:   1024,
			SSDSpace: 1024,
			HDDSpace: 1024,
			Clusters: 1,
		}
		c.RequestChange(r)

		require.Equal(t, Resources{}, c.quota)
		require.Equal(t, Resources{}, c.used)
		require.Equal(t, r, c.diff)
	})

	t.Run("Sub", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		r := Resources{
			CPU:      -1,
			GPU:      -1,
			Memory:   -1024,
			SSDSpace: -1024,
			HDDSpace: -1024,
			Clusters: -1,
		}
		c.RequestChange(r)

		require.Equal(t, Resources{}, c.quota)
		require.Equal(t, Resources{}, c.used)
		require.Equal(t, r, c.diff)
	})
}

func TestConsumption_Diff(t *testing.T) {
	t.Run("Empty", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		require.Equal(t, Resources{}, c.Diff())
	})

	t.Run("NotEmpty", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		r := Resources{
			CPU:      1,
			GPU:      1,
			Memory:   1024,
			SSDSpace: 1024,
			HDDSpace: 1024,
			Clusters: 1,
		}
		c.RequestChange(r)

		require.Equal(t, r, c.Diff())
	})
}

func TestConsumption_Changed(t *testing.T) {
	t.Run("Empty", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		require.False(t, c.Changed())
	})

	t.Run("NotEmpty", func(t *testing.T) {
		c := NewConsumption(Resources{}, Resources{})
		c.RequestChange(Resources{CPU: 1})
		require.True(t, c.Changed())
	})
}

func TestConsumption_Validate(t *testing.T) {
	t.Run("Empty", func(t *testing.T) {
		require.NoError(t, NewConsumption(Resources{}, Resources{}).Validate())
	})

	t.Run("Equal", func(t *testing.T) {
		r := Resources{
			CPU:      1,
			GPU:      1,
			Memory:   1024,
			SSDSpace: 1024,
			HDDSpace: 1024,
			Clusters: 1,
		}
		require.NoError(t, NewConsumption(r, r).Validate())
	})

	t.Run("FailedPrecondition", func(t *testing.T) {
		r := Resources{CPU: 1}
		err := NewConsumption(Resources{}, r).Validate()
		require.Error(t, err)
		require.True(t, semerr.IsFailedPrecondition(err))
	})

	t.Run("ErrorMessage", func(t *testing.T) {
		r := Resources{
			CPU:      1,
			GPU:      1,
			Memory:   1024,
			SSDSpace: 1024,
			HDDSpace: 1024,
			Clusters: 1,
		}
		err := NewConsumption(Resources{}, r).Validate()
		require.Error(t, err)
		require.EqualError(t, err, "Quota limits exceeded, not enough \"cpu: 1, gpu cards: 1, memory: 1.0 KiB, ssd space: 1.0 KiB, hdd space: 1.0 KiB, clusters: 1\". Please contact support to request extra resource quota.")
	})
}
