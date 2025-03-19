package metrics

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/settings"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type adIn struct {
	rs []models.ManagementRequest
}

type adExp struct {
	m MetricAutoDutySuccessRate
}

type adTestCase struct {
	name string
	in   adIn
	exp  adExp
}

func getAdTc() []adTestCase {
	var results = []adTestCase{
		{
			name: "handled by autoduty counts",
			in: adIn{rs: []models.ManagementRequest{{
				Name:       "any-string",
				ResolvedBy: settings.CMSRobotLogin,
				AnalysedBy: settings.CMSRobotLogin}}},
			exp: adExp{m: MetricAutoDutySuccessRate{
				"any-string": &HandledCounter{1, 1},
			}},
		}, {
			name: "zero requests",
			in:   adIn{rs: []models.ManagementRequest{}},
			exp:  adExp{m: MetricAutoDutySuccessRate{}},
		},
	}
	return results
}

func TestAutoDutyMetrics(t *testing.T) {
	for _, tc := range getAdTc() {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			cmsdb := mocks.NewMockClient(ctrl)
			cmsdb.EXPECT().GetRequestsStatInWindow(gomock.Any(), gomock.Any()).Return(tc.in.rs, nil).Times(1)

			l, _ := zap.New(zap.KVConfig(log.DebugLevel))
			minter := Interactor{lg: l, cmsdb: cmsdb}
			m, err := minter.AutoDutySuccessRate(ctx)
			require.NoError(t, err)
			require.Equal(t, tc.exp.m, m)
		})
	}

}

func TestAutoHandledCounter(t *testing.T) {
	t.Run("returns percents", func(t *testing.T) {
		hc := HandledCounter{
			DutyHandled: 3,
			All:         6,
		}
		require.Equal(t, 50.0, hc.Percents())

		hc = HandledCounter{
			DutyHandled: 121,
			All:         121,
		}
		require.Equal(t, 100.0, hc.Percents())

		hc = HandledCounter{
			DutyHandled: 0,
			All:         123,
		}
		require.Equal(t, 0.0, hc.Percents())
	})
}
