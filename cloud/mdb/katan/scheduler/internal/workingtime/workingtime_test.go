package workingtime_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/holidays/weekends"
	"a.yandex-team.ru/cloud/mdb/katan/scheduler/internal/workingtime"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestCheck(t *testing.T) {
	for _, tt := range []struct {
		name string
		ts   string
		ok   bool
	}{
		{
			"Sunday is not a working time",
			"Sun Jan 10 15:04:05 +0300 2021",
			false,
		},
		{
			"Friday is not a working time",
			"Fri Jan 22 15:04:05 +0300 2021",
			false,
		},
		{
			"Monday is a working time",
			"Mon Jan 18 15:04:05 +0300 2021",
			true,
		},
	} {
		t.Run(tt.name, func(t *testing.T) {
			ts, err := time.Parse(time.RubyDate, tt.ts)
			require.NoError(t, err, "parse time sample")
			ok, err := workingtime.Check(context.Background(), &weekends.Calendar{}, ts)

			require.NoError(t, err)
			require.Equal(t, tt.ok, ok)
		})
	}
	t.Run("Not a working time for error", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		cal := mocks.NewMockCalendar(ctrl)
		cal.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, xerrors.New("test error"))
		ok, err := workingtime.Check(context.Background(), cal, time.Time{})

		require.Error(t, err)
		require.False(t, ok)
	})
}
