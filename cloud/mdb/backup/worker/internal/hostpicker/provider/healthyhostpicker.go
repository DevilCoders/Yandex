package hostpicker

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
	healthclient "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/client"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HealthyHostPicker struct {
	health      healthclient.MDBHealthClient
	lg          log.Logger
	healthRetry *retry.BackOff
	cfg         Config
}

func NewHealthyHostPicker(health healthclient.MDBHealthClient, lg log.Logger, cfg Config) *HealthyHostPicker {
	return &HealthyHostPicker{
		health: health,
		lg:     lg,
		healthRetry: retry.New(retry.Config{
			MaxRetries: cfg.Config.HealthMaxRetries,
		}),
		cfg: cfg,
	}
}

func (hp *HealthyHostPicker) PickHost(ctx context.Context, fqdns []string) (string, error) {
	healths, err := hp.health.GetHostsHealth(ctx, fqdns)
	if err != nil {
		return "", err
	}

	statusesHosts := hostStatusMapFromHostHealths(healths)
	for _, status := range hp.cfg.Config.HostHealthStatusesOrder {
		if len(statusesHosts[status]) > 0 {
			ctxlog.Debugf(ctx, hp.lg, "picked %q from %+v", statusesHosts[status][0], healths)
			return statusesHosts[status][0], nil
		}
	}
	return "", xerrors.Errorf("failed to pick suitable host from: %+v", healths)
}

func hostStatusMapFromHostHealths(healths []types.HostHealth) map[types.HostStatus][]string {
	statuses := make(map[types.HostStatus][]string)
	for _, h := range healths {
		statuses[h.Status()] = append(statuses[h.Status()], h.FQDN())
	}
	return statuses
}
