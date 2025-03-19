package workaholic_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/workaholic"
)

func TestCalendar_Range(t *testing.T) {
	toDate := func(value string) time.Time {
		ts, err := time.Parse(time.RubyDate, value)
		require.NoError(t, err)
		return ts
	}
	calendar := &workaholic.Calendar{}
	ret, err := calendar.Range(
		context.Background(),
		toDate("Fri Jan 08 15:04:05 +0300 2021"),
		toDate("Sun Jan 10 15:04:05 +0300 2021"),
	)
	require.NoError(t, err)
	require.Equal(t, []holidays.Day{
		{Date: toDate("Fri Jan 08 00:00:00 +0300 2021"), Type: holidays.Weekday},
		{Date: toDate("Sat Jan 09 00:00:00 +0300 2021"), Type: holidays.Weekday},
		{Date: toDate("Sun Jan 10 00:00:00 +0300 2021"), Type: holidays.Weekday},
	}, ret)
}
