package workaholic

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/internal/date"
)

// Calendar is naive holidays implementations - all day are workdays
type Calendar struct {
}

var _ holidays.Calendar = &Calendar{}

func (n *Calendar) Range(ctx context.Context, from, to time.Time) ([]holidays.Day, error) {
	var ret []holidays.Day
	for _, d := range date.Range(from, to) {
		ret = append(ret, holidays.Day{
			Date: d,
			Type: holidays.Weekday,
		})
	}
	return ret, nil
}
