package workingtime

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Check checks that the given time is a good time for new rollouts.
// Today is a weekday and tomorrow is a weekday too.
func Check(ctx context.Context, cal holidays.Calendar, tm time.Time) (bool, error) {
	calendar, err := cal.Range(ctx, tm, tm.Add(time.Hour*24))
	if err != nil {
		return false, xerrors.Errorf("get calendar: %w", err)
	}

	for _, day := range calendar {
		if day.Type != holidays.Weekday {
			return false, nil
		}
	}
	return true, nil
}
