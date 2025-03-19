package models

import (
	"context"
	"math"
	"math/rand"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	day                    = 24 * time.Hour
	week                   = 7 * day
	MaxDelayDaysUpperBound = 90 * day
)

func IsTimeInMaintenanceWindow(mwDay time.Weekday, upperBoundMWHour int, timeToCheck time.Time) bool {
	// convert to UTC, cause MW settings defined in UTC
	timeToCheck = timeToCheck.In(time.UTC)
	if timeToCheck.Weekday() != mwDay {
		return false
	}

	return upperBoundMWHour-1 == timeToCheck.Hour()
}

// nearestMaintenanceTime returns nearest maintenance time from given time
func nearestMaintenanceTime(mwDay time.Weekday, upperBoundMWHour int, from time.Time) time.Time {
	// convert to UTC, cause MW settings defined in UTC
	from = from.In(time.UTC)
	fromWeekday := from.Weekday()

	mwHour := upperBoundMWHour - 1
	var addDays int
	switch {
	case fromWeekday < mwDay:
		// at that week
		addDays = int(mwDay - fromWeekday)
	case fromWeekday > mwDay:
		// next week
		addDays = 7 - int(fromWeekday-mwDay)
	case fromWeekday == mwDay:
		if from.Hour() < mwHour {
			// that week
			// don't need to add something
		} else {
			// next week
			addDays = 7
		}
	}

	return from.Truncate(24 * time.Hour).
		Add(time.Duration(addDays*24+mwHour) * time.Hour)
}

// nearestWorkTime returns nearest work time on duty can fix failed MW task
// It's better not to schedule tasks on holidays, cause on duty should rest on holidays too.
func nearestWorkTime(ctx context.Context, from time.Time, cal holidays.Calendar) (time.Time, error) {
	calendar, err := cal.Range(ctx, from, from.Add(MaxDelayDaysUpperBound))
	if err != nil {
		return time.Time{}, xerrors.Errorf("get holidays: %w", err)
	}
	var addDays optional.Duration
	for i := 0; i < len(calendar)-1; i++ {
		if calendar[i].Type == holidays.Weekday && calendar[i+1].Type == holidays.Weekday {
			addDays.Set(day * time.Duration(i))
			break
		}
	}
	if !addDays.Valid {
		return time.Time{}, xerrors.Errorf("there are no two sequential working days in a %s-%s interval", from, from.Add(MaxDelayDaysUpperBound))
	}
	from = from.Truncate(time.Hour) // strip minutes to add random delay
	return from.Add(addDays.Duration), nil
}

// PlanMaintenanceTime return nearest maintenance time with minDelay, maxDelay restriction
func PlanMaintenanceTime(ctx context.Context, mw MaintenanceSettings, minDelay time.Duration, now time.Time, maxDelay time.Time, cal holidays.Calendar) (time.Time, error) {
	if maxDelay.IsZero() {
		return time.Time{}, xerrors.Errorf("maxDelay cannot be zero")
	}
	var result time.Time
	var err error
	if !mw.Valid {
		result, err = nearestWorkTime(ctx, now.Add(minDelay), cal)
		if err != nil {
			return time.Time{}, err
		}
	} else {
		result = nearestMaintenanceTime(mw.Weekday, mw.UpperBoundMWHour, now)
		weeksCnt := math.Ceil(float64(minDelay/day) / 7.0)
		if result.Sub(now) < minDelay {
			result = result.Add(week * time.Duration(weeksCnt))
		}
	}
	if result.After(maxDelay) {
		result = maxDelay
	}
	rand.Seed(time.Now().UnixNano())
	return result.Add(randomDelay(time.Now().UnixNano())), nil
}

func randomDelay(seed int64) time.Duration {
	rand.Seed(seed)
	return time.Minute * time.Duration(rand.Intn(59))
}
