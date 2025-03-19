package date_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays/internal/date"
)

func TestRange(t *testing.T) {
	toDate := func(value string) time.Time {
		ts, err := time.Parse(time.RFC3339, value)
		require.NoError(t, err)
		return ts
	}

	t.Run("real range", func(t *testing.T) {
		require.Equal(t, []time.Time{
			toDate("2022-02-28T00:00:00+03:00"),
			toDate("2022-03-01T00:00:00+03:00"),
			toDate("2022-03-02T00:00:00+03:00"),
		}, date.Range(
			toDate("2022-02-28T12:02:03+03:00"),
			toDate("2022-03-02T09:02:03+03:00"),
		))
	})

	t.Run("from and to are equals", func(t *testing.T) {
		require.Equal(t, []time.Time{
			toDate("2022-02-28T00:00:00+03:00"),
		}, date.Range(
			toDate("2022-02-28T12:02:03+03:00"),
			toDate("2022-02-28T13:03:04+03:00"),
		))
	})

	t.Run("from above to", func(t *testing.T) {
		require.Empty(t, date.Range(
			toDate("2022-02-28T12:02:03+03:00"),
			toDate("2022-02-25T13:03:04+03:00"),
		))
	})
}
