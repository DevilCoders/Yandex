package metrics

import (
	"context"

	"github.com/prometheus/client_golang/prometheus"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type dumpHelper struct {
	g      prometheus.Gauge
	source string
}

func (i *Interactor) Dump(ctx context.Context) {
	m, err := i.AutoDutySuccessRate(ctxlog.WithFields(ctx, log.String("metric", "auto duty success rate")))
	if err != nil {
		ctxlog.Errorf(ctx, i.lg, "cannot collect metrics autoduty success rate %v", err)
		return
	}
	i.metrics.autodutyTotal.Set(m.All().Percents())

	dhs := []dumpHelper{
		{i.metrics.autodutyPrepare, models.ManagementRequestActionPrepare},
		{i.metrics.autodutyDeactivate, models.ManagementRequestActionDeactivate},
		{i.metrics.autodutyPowerOff, models.ManagementRequestActionPowerDashOff},
		{i.metrics.autodutyReboot, models.ManagementRequestActionReboot},
		{i.metrics.autodutyProfile, models.ManagementRequestActionProfile},
		{i.metrics.autodutyRedeploy, models.ManagementRequestActionRedeploy},
		{i.metrics.autodutyRepairLink, models.ManagementRequestActionRepairDashLink},
		{i.metrics.autodutyChangeDisk, models.ManagementRequestActionChangeDashDisk},
		{i.metrics.autodutyTemporaryUnreachable, models.ManagementRequestActionTemporaryDashUnreachable},
	}
	for _, dh := range dhs {
		value, ok := m[dh.source]
		if ok {
			dh.g.Set(value.Percents())
		}
	}
}
