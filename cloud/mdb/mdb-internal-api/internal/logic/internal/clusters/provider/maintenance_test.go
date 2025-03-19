package provider

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

func makeMaintenanceInfo(createdAt, delayedUntil time.Time, window clusters.MaintenanceWindow) clusters.MaintenanceInfo {
	operation := clusters.NewMaintenanceOperation(
		"",
		createdAt,
		delayedUntil,
		"",
	)
	operation.NearestMaintenanceWindow = CalculateNearestMaintenanceWindow(delayedUntil, window)
	operation.LatestMaintenanceTime = CalculateLatestMaintenanceTime(createdAt)
	return clusters.MaintenanceInfo{
		Window:    window,
		Operation: operation,
	}
}

func Test_GetNewMaintenanceTime(t *testing.T) {
	layoutISO := "2006-01-02"
	t.Run("NotPlannedYet", func(t *testing.T) {
		delayedUntil, _ := time.Parse(layoutISO, "2000-01-08")

		maintenanceInfo := clusters.MaintenanceInfo{
			Window:    clusters.NewAnytimeMaintenanceWindow(),
			Operation: clusters.MaintenanceOperation{},
		}
		_, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeUnspecified,
			optional.Time{Time: delayedUntil, Valid: true})
		expectedErrorMsg := "There is no maintenance operation at this time"
		assert.EqualErrorf(t, err, expectedErrorMsg, "Error should be: %v, got: %v", expectedErrorMsg, err)
	})

	t.Run("RescheduleTypeUnspecified", func(t *testing.T) {
		delayedUntil, _ := time.Parse(layoutISO, "2000-01-08")
		window := clusters.NewWeeklyMaintenanceWindow("MON", 1)
		maintenanceInfo := makeMaintenanceInfo(delayedUntil, delayedUntil, window)
		_, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeUnspecified,
			optional.Time{Time: delayedUntil, Valid: true})
		expectedErrorMsg := "Reschedule type need to be specified"
		assert.EqualErrorf(t, err, expectedErrorMsg, "Error should be: %v, got: %v", expectedErrorMsg, err)
	})

	t.Run("RescheduleTypeImmediateAfterLatestMaintenanceTime", func(t *testing.T) {
		delayedUntil, _ := time.Parse(layoutISO, "2000-01-08")
		window := clusters.NewWeeklyMaintenanceWindow("MON", 1)
		maintenanceInfo := makeMaintenanceInfo(delayedUntil, delayedUntil, window)
		_, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeImmediate,
			optional.Time{Time: delayedUntil, Valid: true})
		require.NoError(t, err)
	})

	t.Run("RescheduleTypeSpecificTimeEmptyDelayedUntil", func(t *testing.T) {
		delayedUntil, _ := time.Parse(layoutISO, "2000-01-08")
		window := clusters.NewWeeklyMaintenanceWindow("MON", 1)
		maintenanceInfo := makeMaintenanceInfo(delayedUntil, delayedUntil, window)
		_, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeSpecificTime,
			optional.Time{Time: delayedUntil, Valid: false})
		expectedErrorMsg := "Delayed until time need to be specified with specific time rescheduling type"
		assert.EqualErrorf(t, err, expectedErrorMsg, "Error should be: %v, got: %v", expectedErrorMsg, err)
	})

	t.Run("RescheduleTypeSpecificTimeAfterLatestMaintenanceTime", func(t *testing.T) {
		delayedUntil, _ := time.Parse(layoutISO, "2000-01-08")
		window := clusters.NewWeeklyMaintenanceWindow("MON", 1)
		maintenanceInfo := makeMaintenanceInfo(delayedUntil, delayedUntil, window)
		_, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeSpecificTime,
			optional.Time{Time: delayedUntil.AddDate(0, 0, 7*3+1), Valid: true})
		expectedErrorMsg := "Maintenance operation time cannot exceed 2000-01-29T00:00:00Z"
		assert.EqualErrorf(t, err, expectedErrorMsg, "Error should be: %v, got: %v", expectedErrorMsg, err)
	})

	t.Run("RescheduleTypeSpecificTime", func(t *testing.T) {
		createTime, _ := time.Parse(layoutISO, "2000-01-01")
		newPlannedTime, _ := time.Parse(layoutISO, "2000-01-10")
		window := clusters.NewWeeklyMaintenanceWindow("MON", 1)
		maintenanceInfo := makeMaintenanceInfo(createTime, createTime.AddDate(0, 0, 7), window)
		actualNewPlannedTime, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeSpecificTime,
			optional.Time{Time: newPlannedTime, Valid: true})
		require.NoError(t, err)
		require.Equal(t, actualNewPlannedTime, newPlannedTime)
	})

	t.Run("RescheduleTypeNextAvailableWindow", func(t *testing.T) {
		createTime, _ := time.Parse(layoutISO, "2000-01-01")
		newPlannedTime, _ := time.Parse(layoutISO, "2000-01-10")
		window := clusters.NewWeeklyMaintenanceWindow("MON", 1)
		maintenanceInfo := makeMaintenanceInfo(createTime, createTime.AddDate(0, 0, 7), window)
		actualNewPlannedTime, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeNextAvailableWindow,
			optional.Time{})
		require.NoError(t, err)
		require.Equal(t, actualNewPlannedTime, newPlannedTime)
	})

	t.Run("RescheduleTypeNextAvailableWindowAnytimeMaintenanceWindow", func(t *testing.T) {
		createTime, _ := time.Parse(layoutISO, "2000-01-01")
		window := clusters.NewAnytimeMaintenanceWindow()
		maintenanceInfo := makeMaintenanceInfo(createTime, createTime.AddDate(0, 0, 7), window)
		_, err := GetNewMaintenanceTime(
			maintenanceInfo,
			clusters.RescheduleTypeNextAvailableWindow,
			optional.Time{})
		expectedErrorMsg := "The window need to be specified with next available window rescheduling type"
		assert.EqualErrorf(t, err, expectedErrorMsg, "Error should be: %v, got: %v", expectedErrorMsg, err)
	})
}

func TestCalculateNearestMaintenanceWindow(t *testing.T) {
	t.Run("AnytimeMW returns nil", func(t *testing.T) {
		require.Nil(t,
			CalculateNearestMaintenanceWindow(
				time.Date(2020, 1, 1, 2, 0, 0, 0, time.Local),
				clusters.NewAnytimeMaintenanceWindow(),
			),
		)
	})

	operationTimes := []time.Time{
		time.Date(2020, 1, 1, 2, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 0, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 2, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 2, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 5, 23, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 20, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 7, 0, 0, 0, 0, time.UTC),
	}

	windowDays := make([]string, len(operationTimes))
	for i := 0; i < len(operationTimes); i++ {
		windowDays[i] = "MON"
	}

	windowHours := []int{
		2,
		2,
		2,
		2,
		2,
		23,
		23,
		23,
	}

	nextWindowTimes := []time.Time{
		time.Date(2020, 1, 6, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 13, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 13, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 1, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 22, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 6, 22, 0, 0, 0, time.UTC),
		time.Date(2020, 1, 13, 22, 0, 0, 0, time.UTC),
	}

	t.Run("Valid test case size", func(t *testing.T) {
		require.Equal(t, len(operationTimes), len(windowDays))
		require.Equal(t, len(operationTimes), len(windowHours))
		require.Equal(t, len(operationTimes), len(nextWindowTimes))
	})

	for i := 0; i < len(operationTimes); i++ {
		nextWindowTime := CalculateNearestMaintenanceWindow(
			operationTimes[i],
			clusters.NewWeeklyMaintenanceWindow(windowDays[i], windowHours[i]),
		)

		t.Run(fmt.Sprintf("Correctness #%d", i), func(t *testing.T) {
			require.Equal(t, nextWindowTimes[i].Hour(), nextWindowTime.Hour())
			require.Equal(t, nextWindowTimes[i].Day(), nextWindowTime.Day())
			require.Equal(t, nextWindowTimes[i].Month(), nextWindowTime.Month())
			require.Equal(t, nextWindowTimes[i].Year(), nextWindowTime.Year())
		})
	}
}
