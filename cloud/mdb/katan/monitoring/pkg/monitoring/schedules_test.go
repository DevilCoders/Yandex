package monitoring_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb/mocks"
	"a.yandex-team.ru/cloud/mdb/katan/monitoring/pkg/monitoring"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestMonrun_CheckBrokenSchedules(t *testing.T) {
	t.Run("OK when no schedules marching namespace", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := mocks.NewMockKatanDB(ctrl)
		kdb.EXPECT().IsReady(gomock.Any()).Return(nil)
		kdb.EXPECT().Schedules(gomock.Any()).Return(
			[]katandb.Schedule{{Namespace: "B", State: katandb.ScheduleStateActive}},
			nil,
		)

		m, err := monitoring.New(&nop.Logger{}, kdb, monitoring.DefaultConfig())
		require.NoError(t, err)
		require.Equal(t, monrun.Result{}, m.CheckBrokenSchedules(context.Background(), "A", ""))
	})

	t.Run("one schedule is broken", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := mocks.NewMockKatanDB(ctrl)
		kdb.EXPECT().IsReady(gomock.Any()).Return(nil)
		kdb.EXPECT().Schedules(gomock.Any()).Return(
			[]katandb.Schedule{{ID: 10, Name: "highstate", Namespace: "A", State: katandb.ScheduleStateBroken}},
			nil,
		)
		kdb.EXPECT().ClusterRolloutsFailedInSchedule(gomock.Any(), gomock.Any()).Return(
			[]katandb.ClusterRollout{
				{
					ClusterID: "cid14",
					State:     katandb.ClusterRolloutFailed,
					Comment:   optional.NewString("rollout on cluster failed: shipment 9 in unexpected status: UNKNOWN"),
				},
			},
			nil,
		)
		kdb.EXPECT().RolloutShipmentsByCluster(gomock.Any(), gomock.Any(), gomock.Any()).Return(
			[]katandb.RolloutShipment{
				{
					ShipmentID: 7,
					FQDNs:      []string{"myt-1.db.yandex.net", "man-1.db.yandex.net"},
				},
				{
					ShipmentID: 9,
					FQDNs:      []string{"myt-1.db.yandex.net", "man-1.db.yandex.net"},
				},
			},
			nil,
		)

		m, err := monitoring.New(&nop.Logger{}, kdb, monitoring.DefaultConfig())
		require.NoError(t, err)
		require.Equal(t, monrun.Result{
			Code:    monrun.CRIT,
			Message: "highstate schedule (id: 10) is broken. It fails on 1 clusters. ['cid14' cluster cause: rollout on cluster failed: shipment 9 in unexpected status: UNKNOWN. Shipment: 9 ]",
		}, m.CheckBrokenSchedules(context.Background(), "A", ""))
	})

	t.Run("one schedule is broken (links to UI)", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := mocks.NewMockKatanDB(ctrl)
		kdb.EXPECT().IsReady(gomock.Any()).Return(nil)
		kdb.EXPECT().Schedules(gomock.Any()).Return(
			[]katandb.Schedule{{ID: 10, Name: "highstate", Namespace: "A", State: katandb.ScheduleStateBroken}},
			nil,
		)
		kdb.EXPECT().ClusterRolloutsFailedInSchedule(gomock.Any(), gomock.Any()).Return(
			[]katandb.ClusterRollout{
				{
					ClusterID: "cid14",
					State:     katandb.ClusterRolloutFailed,
					Comment:   optional.NewString("rollout on cluster failed: shipment 691908 in unexpected status: ERROR"),
				},
			},
			nil,
		)
		kdb.EXPECT().RolloutShipmentsByCluster(gomock.Any(), gomock.Any(), gomock.Any()).Return(
			[]katandb.RolloutShipment{
				{
					ShipmentID: 691908,
					FQDNs:      []string{"vla-x0b1wcor6qs21g4v.db.yandex.net"},
				},
			},
			nil,
		)

		m, err := monitoring.New(&nop.Logger{}, kdb, monitoring.DefaultConfig())
		require.NoError(t, err)
		require.Equal(t, monrun.Result{
			Code:    monrun.CRIT,
			Message: "highstate schedule (id: 10) is broken. It fails on 1 clusters. ['cid14' cluster cause: rollout on cluster failed: shipment 691908 in unexpected status: ERROR. Shipment: https://ui.db.yandex-team.ru/deploy/shipment/691908 ]",
		}, m.CheckBrokenSchedules(context.Background(), "A", "https://ui.db.yandex-team.ru"))
	})

	t.Run("two schedules are broken", func(t *testing.T) {
		ctrl := gomock.NewController(t)
		defer ctrl.Finish()
		kdb := mocks.NewMockKatanDB(ctrl)
		kdb.EXPECT().IsReady(gomock.Any()).Return(nil)
		kdb.EXPECT().Schedules(gomock.Any()).Return(
			[]katandb.Schedule{
				{ID: 10, Name: "errata", Namespace: "A", State: katandb.ScheduleStateBroken},
				{ID: 12, Name: "errata", Namespace: "A", State: katandb.ScheduleStateBroken},
			},
			nil,
		)
		kdb.EXPECT().ClusterRolloutsFailedInSchedule(gomock.Any(), gomock.Any()).Return(
			nil,
			nil,
		).AnyTimes()

		m, err := monitoring.New(&nop.Logger{}, kdb, monitoring.DefaultConfig())
		require.NoError(t, err)
		ret := m.CheckBrokenSchedules(context.Background(), "A", "")
		require.Equal(t, monrun.CRIT, ret.Code)
		require.Regexp(t, `^2 schedules are broken\. errata schedule .*`, ret.Message)
	})
}
