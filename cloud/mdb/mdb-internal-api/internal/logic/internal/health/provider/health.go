package provider

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/log"
)

func (h *Health) Cluster(ctx context.Context, cid string) (clusters.Health, error) {
	cluster, err := h.client.GetClusterHealth(ctx, cid)
	health := clusters.HealthUnknown
	if err == nil {
		health, err = clusterHealthFromClient(cluster.Status)
	}

	if err != nil {
		msg := "cannot retrieve cluster health"
		if sentry.NeedReport(err) {
			h.l.Error(msg, log.Error(err))
			sentry.GlobalClient().CaptureError(ctx, err, nil)
		} else {
			h.l.Warn(msg, log.Error(err))
		}
	}

	return health, err
}

func (h *Health) Hosts(ctx context.Context, fqdns []string) (map[string]hosts.Health, error) {
	hs, err := h.client.GetHostsHealth(ctx, fqdns)
	if err != nil {
		// TODO: semerr
		return nil, err
	}

	res := make(map[string]hosts.Health, len(hs))
	for _, host := range hs {
		health, err := hostHealthFromClient(host)
		if err != nil {
			h.l.Error(fmt.Sprintf("cannot retrieve host %s health", host.FQDN()), log.Error(err))
			sentry.GlobalClient().CaptureError(ctx, err, nil)
			continue
		}

		res[host.FQDN()] = health
	}

	return res, nil
}
