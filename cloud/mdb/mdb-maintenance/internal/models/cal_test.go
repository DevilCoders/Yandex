package models

import (
	"context"
	"math"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays/weekends"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/workaholic"
)

func TestNearestMaintenanceTime(t *testing.T) {
	//	  January 2006
	// Su Mo Tu We Th Fr Sa
	//  1  2  3  4  5  6  7
	//  8  9 10 11 12 13 14
	// 15 16 17 18 19 20 21
	// 22 23 24 25 26 27 28
	// 29 30 31
	//

	now, err := time.Parse(time.RFC850, "Monday, 02-Jan-06 15:04:05 UTC")
	require.NoError(t, err)

	makeExpectedTime := func(day, hour int) time.Time {
		return time.Date(
			now.Year(), now.Month(), day, hour, 0, 0, 0, time.UTC,
		)
	}
	tests := []struct {
		name             string
		mwDay            time.Weekday
		upperBoundMWHour int
		expected         time.Time
	}{
		{
			"MW-day has not come yet",

			time.Wednesday,
			10,
			makeExpectedTime(4, 9),
		},
		{
			"MW-day has already passed",
			time.Sunday,
			22,
			makeExpectedTime(8, 21),
		},
		{
			"MW-day is today, MW-hour has not come yet",
			time.Monday,
			17,
			makeExpectedTime(2, 16),
		},
		{
			"MW-day is today, MW-hour start",
			time.Monday,
			15,
			makeExpectedTime(9, 14),
		},
		{
			"MW-day is today, MW-hour passed",
			time.Monday,
			10,
			makeExpectedTime(9, 9),
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.expected, nearestMaintenanceTime(tt.mwDay, tt.upperBoundMWHour, now))
		})
	}
}

func TestIsTimeInMaintenanceWindow(t *testing.T) {
	mwDay := time.Tuesday
	upperBoundMWHour := 16
	tests := []struct {
		name        string
		timeToCheck string
		expected    bool
	}{
		{
			"Day of ts before mw",
			"Monday, 02-Jan-06 15:59:05 UTC",
			false,
		},
		{
			"Hour of ts before mw",
			"Tuesday, 03-Jan-06 14:59:05 UTC",
			false,
		},
		{
			"Ts in mw",
			"Tuesday, 03-Jan-06 15:59:05 UTC",
			true,
		},
		{
			"Hour of ts after mw",
			"Tuesday, 03-Jan-06 16:00:05 UTC",
			false,
		},
		{
			"Day of ts after mw",
			"Wednesday, 04-Jan-06 15:59:05 UTC",
			false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			ts, err := time.Parse(time.RFC850, tt.timeToCheck)
			require.NoError(t, err)
			require.Equal(t, tt.expected, IsTimeInMaintenanceWindow(mwDay, upperBoundMWHour, ts))
		})
	}
}

func TestPlanMWTime(t *testing.T) {
	now, _ := time.Parse(time.RFC850, "Monday, 02-Jan-06 15:04:05 UTC")

	daysUntil := func(t time.Time) float64 {
		return t.Sub(now).Hours() / 24
	}

	t.Run("minDays is more than week and we respect that", func(t *testing.T) {
		ts := now.Add(2 * time.Hour) // ordered number of hour to use with settings
		ret, err := PlanMaintenanceTime(
			context.Background(),
			NewMaintenanceSettings(ts.Weekday(), ts.Hour()),
			9*day,
			now,
			now.Add(21*24*time.Hour),
			&workaholic.Calendar{})
		require.NoError(t, err)
		dUntil := daysUntil(ret)
		require.LessOrEqual(t, 14.0, dUntil, "ret %s should be in >=14 days", ret)
		require.LessOrEqual(t, dUntil, 15.0, "ret %s should be in <15 days", ret)
	})

	t.Run("MW is set before minDays and maxDays more than week from planned date", func(t *testing.T) {
		ts := now.Add(2 * time.Hour) // ordered number of hour to use with settings
		ret, err := PlanMaintenanceTime(context.Background(), NewMaintenanceSettings(ts.Weekday(), ts.Hour()), day, now, now.Add(8*24*time.Hour), &workaholic.Calendar{})
		require.NoError(t, err)
		dUntil := daysUntil(ret)
		require.LessOrEqual(t, 7.0, dUntil, "ret %s should be in >=7 days", ret)
		require.Less(t, dUntil, 8.0, "ret %s should be in <8 days", ret)
	})

	t.Run("MW is set before minDays and maxDays before than a week from planned date", func(t *testing.T) {
		ts := now.Add(2 * time.Hour) // ordered number of hour to use with settings
		ret, err := PlanMaintenanceTime(context.Background(), NewMaintenanceSettings(ts.Weekday(), ts.Hour()), day, now, now.Add(5*24*time.Hour), &workaholic.Calendar{})
		require.NoError(t, err)
		dUntil := daysUntil(ret)
		require.LessOrEqual(t, 5.0, dUntil, "ret %s should be in >=5 days", ret)
		require.Less(t, dUntil, 6.0, "ret %s should be in <6 days", ret)
	})

	t.Run("planned at next week, cause mw time below minDays", func(t *testing.T) {
		ts := now.Add(2 * time.Hour) // ordered number of hour to use with settings
		ret, err := PlanMaintenanceTime(context.Background(), NewMaintenanceSettings(ts.Weekday(), ts.Hour()), week, now, now.Add(21*24*time.Hour), &workaholic.Calendar{})
		require.NoError(t, err)
		require.LessOrEqual(t, 7.0, daysUntil(ret), "ret %s should be at next week", ret)
	})

	t.Run("wanted task in 25h and 4 minDays from now, sot planned for next MW", func(t *testing.T) {
		ts := now.Add(time.Hour * 25)
		ret, err := PlanMaintenanceTime(context.Background(), NewMaintenanceSettings(ts.Weekday(), ts.Hour()), 4*day, now, now.Add(21*24*time.Hour), &workaholic.Calendar{})
		require.NoError(t, err)
		require.Less(t, daysUntil(ret), 9.0)
		require.Greater(t, daysUntil(ret), 8.0)
	})

	t.Run("unset MW settings planned to next week", func(t *testing.T) {
		ret, err := PlanMaintenanceTime(context.Background(), MaintenanceSettings{}, week, now, now.Add(21*24*time.Hour), &workaholic.Calendar{})
		require.NoError(t, err)
		require.Equal(t, 7.0, math.Round(daysUntil(ret)))
	})

	type TestCase struct {
		name         string
		now          string
		expected     string
		maxDelayDays time.Duration
	}

	for _, tt := range []TestCase{
		{
			name:     "MW time falls on saturday",
			now:      "Sat Oct 16 15:04:05 +0300 2021",
			expected: "Mon Oct 25 15:04:05 +0300 2021",
		},
		{
			name:     "MW time falls on friday",
			now:      "Sat Oct 15 15:04:05 +0300 2021",
			expected: "Mon Oct 25 15:04:05 +0300 2021",
		},
		{
			name:         "MW is on saturday, but Max Delay is stronger",
			now:          "Sat Oct 15 15:04:05 +0300 2021",
			expected:     "Mon Oct 16 15:04:05 +0300 2021",
			maxDelayDays: 1,
		},
	} {
		t.Run(tt.name, func(t *testing.T) {
			nowT, err := time.Parse(time.RubyDate, tt.now)
			require.NoError(t, err, "parse now")

			expected, err := time.Parse(time.RubyDate, tt.expected)
			require.NoError(t, err, "parse expected")

			if tt.maxDelayDays == 0 {
				tt.maxDelayDays = 21
			}
			maxDelay := nowT.Add(24 * time.Hour * tt.maxDelayDays)

			got, err := PlanMaintenanceTime(context.Background(), MaintenanceSettings{}, week, nowT, maxDelay, &weekends.Calendar{})
			require.NoError(t, err)
			require.Equal(t, expected.Truncate(time.Hour), got.Truncate(time.Hour))
		})
	}
}

func Test_randomDelay(t *testing.T) {
	got := randomDelay(1)

	require.True(t, got < time.Hour)
	require.True(t, got > time.Minute)
}
