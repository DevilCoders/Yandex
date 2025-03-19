package clusters

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestMaintenanceWindow_Validate(t *testing.T) {
	t.Run("Any time maintenance window is valid", func(t *testing.T) {
		require.NoError(t, NewAnytimeMaintenanceWindow().Validate())
	})
	t.Run("WeeklyMaintenanceWindow is valid", func(t *testing.T) {
		require.NoError(t, NewWeeklyMaintenanceWindow("MON", 1).Validate())
	})
	t.Run("WeeklyMaintenanceWindow should have hour is range 1-24", func(t *testing.T) {
		err := NewWeeklyMaintenanceWindow("MON", 0).Validate()
		assert.EqualError(t, err, "Maintenance window hour should be in range (1 - 24)")

		err = NewWeeklyMaintenanceWindow("MON", 25).Validate()
		assert.EqualError(t, err, "Maintenance window hour should be in range (1 - 24)")
	})
}

func TestMaintenanceWindow_Anytime(t *testing.T) {
	t.Run("Anytime() returns true for anytime maintenance window", func(t *testing.T) {
		require.True(t, NewAnytimeMaintenanceWindow().Anytime())
	})
	t.Run("Default maintenance window is anytime", func(t *testing.T) {
		require.True(t, MaintenanceWindow{}.Anytime())
	})
	t.Run("WeeklyMaintenanceWindow is not anytime", func(t *testing.T) {
		require.False(t, NewWeeklyMaintenanceWindow("MON", 1).Anytime())
	})
}

func TestMaintenanceWindow_Equal(t *testing.T) {
	weekly := MaintenanceWindow{WeeklyMaintenanceWindow: &WeeklyMaintenanceWindow{Day: "MON", Hour: 1}}
	any := MaintenanceWindow{}
	assert.True(t, weekly.Equal(weekly))
	assert.True(t, weekly.Equal(MaintenanceWindow{WeeklyMaintenanceWindow: &WeeklyMaintenanceWindow{Day: "MON", Hour: 1}}))
	assert.True(t, any.Equal(any))
	assert.True(t, any.Equal(MaintenanceWindow{WeeklyMaintenanceWindow: nil}))
	assert.False(t, weekly.Equal(MaintenanceWindow{WeeklyMaintenanceWindow: &WeeklyMaintenanceWindow{Day: "MON", Hour: 2}}))
	assert.False(t, weekly.Equal(MaintenanceWindow{WeeklyMaintenanceWindow: &WeeklyMaintenanceWindow{Day: "TUE", Hour: 1}}))
	assert.False(t, weekly.Equal(MaintenanceWindow{WeeklyMaintenanceWindow: nil}))
	assert.False(t, weekly.Equal(any))
	assert.False(t, any.Equal(weekly))
}
