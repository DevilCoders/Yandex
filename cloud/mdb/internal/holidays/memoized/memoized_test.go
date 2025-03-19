package memoized_test

import (
	"context"
	"errors"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/memoized"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/mocks"
)

func TestCalendar(t *testing.T) {
	t.Run("memoize Range calls", func(t *testing.T) {
		mustRFC3339 := func(value string) time.Time {
			ret, err := time.Parse(time.RFC3339, value)
			require.NoError(t, err)
			return ret
		}
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		calendar := mocks.NewMockCalendar(ctrl)
		jan1 := mustRFC3339("2021-01-01T00:00:00Z")
		jan2 := mustRFC3339("2021-01-02T00:00:00Z")
		expected := []holidays.Day{
			{Date: jan1, Type: holidays.Holiday},
			{Date: jan2, Type: holidays.Holiday},
		}
		calendar.EXPECT().Range(gomock.Any(), jan1, jan2).Return(expected, nil)

		memo := memoized.New(calendar)

		ret, err := memo.Range(context.Background(), mustRFC3339("2021-01-01T15:15:00Z"), mustRFC3339("2021-01-02T01:11:11Z"))
		require.NoError(t, err)
		require.Equal(t, expected, ret)

		ret, err = memo.Range(context.Background(), mustRFC3339("2021-01-01T15:15:00Z"), mustRFC3339("2021-01-02T02:22:22Z"))
		require.NoError(t, err)
		require.Equal(t, expected, ret)
	})

	t.Run("don't memoize errors", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		calendar := mocks.NewMockCalendar(ctrl)
		t1 := time.Now()
		t2 := time.Now().Add(time.Hour * 24)
		expected := []holidays.Day{{Date: t1}, {Date: t2}}
		calendar.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, errors.New("just flap"))
		calendar.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(expected, nil)

		memo := memoized.New(calendar)

		_, err := memo.Range(context.Background(), t1, t2)
		require.Error(t, err)

		ret, err := memo.Range(context.Background(), t1, t2)
		require.NoError(t, err)
		require.Equal(t, expected, ret)
	})
}
