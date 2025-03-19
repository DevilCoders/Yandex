package snapshot

import (
	"context"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
)

////////////////////////////////////////////////////////////////////////////////

type factory struct {
	config      *snapshot_config.ClientConfig
	credentials auth.Credentials
	metrics     clientMetrics
}

func (f *factory) CreateClient(ctx context.Context) (Client, error) {
	return newClient(ctx, f.config, f.config.GetDefaultZoneId(), f.credentials, f.metrics)
}

func (f *factory) CreateClientFromZone(
	ctx context.Context,
	zoneID string,
) (Client, error) {

	return newClient(ctx, f.config, zoneID, f.credentials, f.metrics)
}

////////////////////////////////////////////////////////////////////////////////

func CreateFactoryWithCreds(
	config *snapshot_config.ClientConfig,
	creds auth.Credentials,
	metricsRegistry metrics.Registry,
) Factory {

	if config.GetDisableAuthentication() {
		creds = nil
	}

	clientMetrics := clientMetrics{
		registry: metricsRegistry,
		errors:   metricsRegistry.CounterVec("errors", []string{"code"}),
	}

	return &factory{
		config:      config,
		credentials: creds,
		metrics:     clientMetrics,
	}
}

func CreateFactory(
	config *snapshot_config.ClientConfig,
	metricsRegistry metrics.Registry,
) Factory {

	return CreateFactoryWithCreds(config, nil, metricsRegistry)
}
