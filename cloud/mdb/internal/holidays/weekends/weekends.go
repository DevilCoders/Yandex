package weekends

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/internal/date"
)

// Calendar is naive holidays implementations it returns only weekends.
// Useful in test or in cases where we don't have access to y-t calendar.
type Calendar struct {
}

var _ holidays.Calendar = &Calendar{}

func (n *Calendar) Range(ctx context.Context, from, to time.Time) ([]holidays.Day, error) {
	var ret []holidays.Day
	for _, d := range date.Range(from, to) {
		dateType := holidays.Weekday
		if weekday := d.Weekday(); weekday == time.Sunday || weekday == time.Saturday {
			dateType = holidays.Weekend
		}
		ret = append(ret, holidays.Day{
			Date: d, Type: dateType,
		})
	}
	return ret, nil
}
