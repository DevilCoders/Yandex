package weekends_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/weekends"
)

func TestCalendar_Range(t *testing.T) {
	parseRuby := func(value string) time.Time {
		ts, err := time.Parse(time.RubyDate, value)
		require.NoError(t, err)
		return ts
	}
	calendar := &weekends.Calendar{}
	ret, err := calendar.Range(
		context.Background(),
		parseRuby("Thu Jan 07 15:04:05 +0300 2021"),
		parseRuby("Sun Jan 10 15:04:05 +0300 2021"),
	)
	require.NoError(t, err)
	require.Equal(t, []holidays.Day{
		{Date: parseRuby("Thu Jan 07 00:00:00 +0300 2021"), Type: holidays.Weekday},
		{Date: parseRuby("Fri Jan 08 00:00:00 +0300 2021"), Type: holidays.Weekday},
		{Date: parseRuby("Sat Jan 09 00:00:00 +0300 2021"), Type: holidays.Weekend},
		{Date: parseRuby("Sun Jan 10 00:00:00 +0300 2021"), Type: holidays.Weekend},
	}, ret)
}
