package scheduler_test

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/holidays"
	calmocks "a.yandex-team.ru/cloud/mdb/internal/holidays/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	katandbmock "a.yandex-team.ru/cloud/mdb/katan/internal/katandb/mocks"
	"a.yandex-team.ru/cloud/mdb/katan/scheduler/internal/scheduler"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestScheduler_ProcessSchedules(t *testing.T) {
	workdays := []holidays.Day{{Type: holidays.Weekday}, {Type: holidays.Weekday}}
	t.Run("Mark schedule as broken if more then MaxFailsSequentially rolls fails sequentially", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := katandbmock.NewMockKatanDB(ctrl)
		cal := calmocks.NewMockCalendar(ctrl)
		kdb.EXPECT().Schedules(gomock.Any()).Return(
			[]katandb.Schedule{
				{
					ID:                42,
					State:             katandb.ScheduleStateActive,
					ExaminedRolloutID: optional.NewInt64(1),
				},
			},
			nil,
		)
		kdb.EXPECT().LastRolloutsBySchedule(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(
			[]katandb.Rollout{
				{
					ID:         10,
					FinishedAt: optional.NewTime(time.Now()),
				},
				{
					ID:         20,
					FinishedAt: optional.NewTime(time.Now()),
				},
			},
			nil,
		)
		kdb.EXPECT().ClusterRollouts(gomock.Any(), int64(10)).Return(
			[]katandb.ClusterRollout{
				{State: katandb.ClusterRolloutSucceeded, ClusterID: "c1"},
				{State: katandb.ClusterRolloutSucceeded, ClusterID: "c2"},
				{State: katandb.ClusterRolloutFailed, ClusterID: "c3"},
				{State: katandb.ClusterRolloutCancelled, ClusterID: "c4"},
			},
			nil,
		)
		kdb.EXPECT().ClusterRollouts(gomock.Any(), int64(20)).Return(
			[]katandb.ClusterRollout{
				{State: katandb.ClusterRolloutFailed, ClusterID: "c5"},
				{State: katandb.ClusterRolloutFailed, ClusterID: "c6"},
				{State: katandb.ClusterRolloutCancelled, ClusterID: "c7"},
			},
			nil,
		)
		kdb.EXPECT().MarkSchedule(gomock.Any(), int64(42), katandb.ScheduleStateBroken, gomock.Any(), gomock.Any()).Return(nil)
		cal.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(workdays, nil)

		cfg := scheduler.DefaultConfig()
		cfg.MaxFailsSequentially = 3
		cfg.MaxFailsInWindow = 100
		s := scheduler.New(kdb, cal, cfg, &nop.Logger{})
		require.NoError(t, s.ProcessSchedules(context.Background()))
	})

	t.Run("Mark schedule as broken if more then MaxFailsInWindow rolls fails", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := katandbmock.NewMockKatanDB(ctrl)
		cal := calmocks.NewMockCalendar(ctrl)
		kdb.EXPECT().Schedules(gomock.Any()).Return(
			[]katandb.Schedule{
				{
					ID:                42,
					State:             katandb.ScheduleStateActive,
					ExaminedRolloutID: optional.NewInt64(1),
				},
			},
			nil,
		)
		kdb.EXPECT().LastRolloutsBySchedule(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(
			[]katandb.Rollout{
				{
					ID:         10,
					FinishedAt: optional.NewTime(time.Now()),
				},
				{
					ID:         20,
					FinishedAt: optional.NewTime(time.Now()),
				},
			},
			nil,
		)
		kdb.EXPECT().ClusterRollouts(gomock.Any(), int64(10)).Return(
			[]katandb.ClusterRollout{
				{State: katandb.ClusterRolloutSucceeded, ClusterID: "c1"},
				{State: katandb.ClusterRolloutFailed, ClusterID: "c2"},
				{State: katandb.ClusterRolloutFailed, ClusterID: "c3"},
				{State: katandb.ClusterRolloutSucceeded, ClusterID: "c4"},
			},
			nil,
		)
		kdb.EXPECT().ClusterRollouts(gomock.Any(), int64(20)).Return(
			[]katandb.ClusterRollout{
				{State: katandb.ClusterRolloutFailed, ClusterID: "c5"},
				{State: katandb.ClusterRolloutFailed, ClusterID: "c6"},
				{State: katandb.ClusterRolloutSucceeded, ClusterID: "c7"},
				{State: katandb.ClusterRolloutFailed, ClusterID: "c7"},
			},
			nil,
		)
		kdb.EXPECT().MarkSchedule(gomock.Any(), int64(42), katandb.ScheduleStateBroken, gomock.Any(), gomock.Any()).Return(nil)
		cal.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(workdays, nil)

		cfg := scheduler.DefaultConfig()
		cfg.MaxFailsSequentially = 100
		cfg.MaxFailsInWindow = 5
		s := scheduler.New(kdb, cal, cfg, &nop.Logger{})
		require.NoError(t, s.ProcessSchedules(context.Background()))
	})

	t.Run("Scheduler doesn't work on holidays", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := katandbmock.NewMockKatanDB(ctrl)
		cal := calmocks.NewMockCalendar(ctrl)
		s := scheduler.New(kdb, cal, scheduler.DefaultConfig(), &nop.Logger{})
		cal.EXPECT().Range(gomock.Any(), gomock.Any(), gomock.Any()).Return(
			[]holidays.Day{{Type: holidays.Holiday}, {Type: holidays.Holiday}}, nil)

		require.NoError(t, s.ProcessSchedules(context.Background()))
	})
}
