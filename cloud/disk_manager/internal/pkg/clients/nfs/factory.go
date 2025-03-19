package nfs

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	nfs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	nfs_client "a.yandex-team.ru/cloud/filestore/public/sdk/go/client"
)

////////////////////////////////////////////////////////////////////////////////

type errorLogger struct {
}

func (l *errorLogger) Print(ctx context.Context, v ...interface{}) {
	logging.Error(ctx, fmt.Sprint(v...))
}

func (l *errorLogger) Printf(ctx context.Context, format string, v ...interface{}) {
	logging.Error(ctx, fmt.Sprintf(format, v...))
}

////////////////////////////////////////////////////////////////////////////////

type warnLogger struct {
}

func (l *warnLogger) Print(ctx context.Context, v ...interface{}) {
	logging.Warn(ctx, fmt.Sprint(v...))
}

func (l *warnLogger) Printf(ctx context.Context, format string, v ...interface{}) {
	logging.Warn(ctx, fmt.Sprintf(format, v...))
}

////////////////////////////////////////////////////////////////////////////////

type infoLogger struct {
}

func (l *infoLogger) Print(ctx context.Context, v ...interface{}) {
	logging.Info(ctx, fmt.Sprint(v...))
}

func (l *infoLogger) Printf(ctx context.Context, format string, v ...interface{}) {
	logging.Info(ctx, fmt.Sprintf(format, v...))
}

////////////////////////////////////////////////////////////////////////////////

type debugLogger struct {
}

func (l *debugLogger) Print(ctx context.Context, v ...interface{}) {
	logging.Debug(ctx, fmt.Sprint(v...))
}

func (l *debugLogger) Printf(ctx context.Context, format string, v ...interface{}) {
	logging.Debug(ctx, fmt.Sprintf(format, v...))
}

////////////////////////////////////////////////////////////////////////////////

type nfsClientLogWrapper struct {
	level   nfs_client.LogLevel
	loggers []nfs_client.Logger
}

func (w *nfsClientLogWrapper) Logger(level nfs_client.LogLevel) nfs_client.Logger {
	if level <= w.level {
		return w.loggers[level]
	}
	return nil
}

func NewNfsClientLog(level nfs_client.LogLevel) nfs_client.Log {
	loggers := []nfs_client.Logger{
		&errorLogger{},
		&warnLogger{},
		&infoLogger{},
		&debugLogger{},
	}
	return &nfsClientLogWrapper{level, loggers}
}

////////////////////////////////////////////////////////////////////////////////

type factory struct {
	config      *nfs_config.ClientConfig
	credentials auth.Credentials
	metrics     clientMetrics
}

func (f *factory) CreateClient(
	ctx context.Context,
	zoneID string,
) (Client, error) {

	zone, ok := f.config.GetZones()[zoneID]
	if !ok {
		// This should be interpreted as non retriable error by task infrastructure.
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf("unknown zoneID %v", zoneID),
		}
	}

	clientCreds := &nfs_client.ClientCredentials{
		RootCertsFile: f.config.GetRootCertsFile(),
		IAMClient:     f.credentials,
	}

	if f.config.GetInsecure() {
		clientCreds = nil
	}

	nfs, err := nfs_client.NewClient(
		&nfs_client.GrpcClientOpts{
			Endpoint:    zone.Endpoints[0],
			Credentials: clientCreds,
		},
		&nfs_client.DurableClientOpts{
			OnError: func(err nfs_client.ClientError) {
				f.metrics.OnError(err)
			},
		},
		NewNfsClientLog(nfs_client.LOG_DEBUG),
	)

	if err != nil {
		return nil, &errors.RetriableError{
			Err: err,
		}
	}

	return &client{
		nfs:     nfs,
		metrics: f.metrics,
	}, nil
}

func (f *factory) CreateClientFromDefaultZone(
	ctx context.Context,
) (Client, error) {

	var zoneID string

	for id := range f.config.GetZones() {
		zoneID = id
		// It's OK to use first known zone.
		break
	}

	return f.CreateClient(ctx, zoneID)
}

////////////////////////////////////////////////////////////////////////////////

func CreateFactoryWithCreds(
	config *nfs_config.ClientConfig,
	creds auth.Credentials,
	metricsRegistry metrics.Registry,
) Factory {

	if config.GetDisableAuthentication() {
		creds = nil
	}

	clientMetrics := clientMetrics{
		registry: metricsRegistry,
		errors:   metricsRegistry.Counter("errors"),
	}

	return &factory{
		config:      config,
		credentials: creds,
		metrics:     clientMetrics,
	}
}

func CreateFactory(
	config *nfs_config.ClientConfig,
	metricsRegistry metrics.Registry,
) Factory {

	return CreateFactoryWithCreds(config, nil, metricsRegistry)
}
