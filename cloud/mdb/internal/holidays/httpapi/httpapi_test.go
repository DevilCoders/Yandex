package httpapi

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
)

func TestParseHolidaysResponse(t *testing.T) {
	t.Run("real response", func(t *testing.T) {
		got, err := parseHolidaysResponse([]byte(`
{
  "holidays": [
	{
      "date": "2020-12-27",
      "type": "weekend",
      "name": "День спасателя"
    },
    {
      "date": "2020-12-28",
      "type": "weekday"
    },
    {
      "date": "2020-12-29",
      "type": "weekday"
    },
    {
      "date": "2020-12-30",
      "type": "weekday"
    },
    {
      "date": "2020-12-31",
      "type": "holiday",
      "name": "Новый год"
    },
    {
      "date": "2021-01-01",
      "type": "holiday",
      "name": "Новогодние каникулы"
    }
  ]
}`))

		newDay := func(ts string, dayType holidays.DayType) holidays.Day {
			et, err := time.Parse("2006-01-02", ts)
			require.NoError(t, err)
			return holidays.Day{
				Date: et,
				Type: dayType,
			}
		}
		require.NoError(t, err)
		expected := []holidays.Day{
			newDay("2020-12-27", holidays.Weekend),
			newDay("2020-12-28", holidays.Weekday),
			newDay("2020-12-29", holidays.Weekday),
			newDay("2020-12-30", holidays.Weekday),
			newDay("2020-12-31", holidays.Holiday),
			newDay("2021-01-01", holidays.Holiday),
		}
		require.Equal(t, expected, got)
	})
	t.Run("empty response", func(t *testing.T) {
		got, err := parseHolidaysResponse([]byte(`{"holidays": []}`))
		require.NoError(t, err)
		require.Empty(t, got)
	})
	t.Run("response without holidays", func(t *testing.T) {
		got, err := parseHolidaysResponse([]byte(`{}`))
		require.Error(t, err)
		require.Empty(t, got)
	})
	t.Run("non json response", func(t *testing.T) {
		got, err := parseHolidaysResponse([]byte(`<holidays/>`))
		require.Error(t, err)
		require.Empty(t, got)
	})
}
