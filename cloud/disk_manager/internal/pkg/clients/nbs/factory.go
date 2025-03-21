package nbs

import (
	"context"
	"fmt"
	"google.golang.org/grpc"
	"google.golang.org/grpc/keepalive"
	"time"

	nbs_client "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	nbs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
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

type nbsClientLogWrapper struct {
	level   nbs_client.LogLevel
	loggers []nbs_client.Logger
}

func (w *nbsClientLogWrapper) Logger(level nbs_client.LogLevel) nbs_client.Logger {
	if level <= w.level {
		return w.loggers[level]
	}

	return nil
}

func NewNbsClientLog(level nbs_client.LogLevel) nbs_client.Log {
	loggers := []nbs_client.Logger{
		&errorLogger{},
		&warnLogger{},
		&infoLogger{},
		&debugLogger{},
	}
	return &nbsClientLogWrapper{level, loggers}
}

////////////////////////////////////////////////////////////////////////////////

type factory struct {
	config      *nbs_config.ClientConfig
	credentials auth.Credentials
	metrics     *clientMetrics
	clients     map[string]Client
}

func (f *factory) initClients(
	ctx context.Context,
) error {

	f.clients = make(map[string]Client)

	dialer := grpc.DialContext

	if f.config.GetGrpcKeepAlive() != nil {
		keepAliveTime, err := time.ParseDuration(
			f.config.GetGrpcKeepAlive().GetTime(),
		)
		if err != nil {
			return err
		}

		keepAliveTimeout, err := time.ParseDuration(
			f.config.GetGrpcKeepAlive().GetTimeout(),
		)
		if err != nil {
			return err
		}

		dialer = func(
			ctx context.Context,
			target string,
			opts ...grpc.DialOption,
		) (conn *grpc.ClientConn, err error) {
			ka := keepalive.ClientParameters{
				Time:                keepAliveTime,
				Timeout:             keepAliveTimeout,
				PermitWithoutStream: f.config.GetGrpcKeepAlive().GetPermitWithoutStream(),
			}
			opts = append(opts, grpc.WithKeepaliveParams(ka))
			return grpc.DialContext(ctx, target, opts...)
		}
	}

	durableClientTimeout, err := time.ParseDuration(
		f.config.GetDurableClientTimeout(),
	)
	if err != nil {
		return err
	}

	discoveryClientHardTimeout, err := time.ParseDuration(
		f.config.GetDiscoveryClientHardTimeout(),
	)
	if err != nil {
		return err
	}

	discoveryClientSoftTimeout, err := time.ParseDuration(
		f.config.GetDiscoveryClientSoftTimeout(),
	)
	if err != nil {
		return err
	}

	for zoneID, zone := range f.config.GetZones() {
		clientCreds := &nbs_client.ClientCredentials{
			RootCertsFile: f.config.GetRootCertsFile(),
			IAMClient:     f.credentials,
		}

		if f.config.GetInsecure() {
			clientCreds = nil
		}

		nbs, err := nbs_client.NewDiscoveryClient(
			zone.Endpoints,
			&nbs_client.GrpcClientOpts{
				Credentials: clientCreds,
				DialContext: dialer,
			},
			&nbs_client.DurableClientOpts{
				Timeout: &durableClientTimeout,
				OnError: func(err nbs_client.ClientError) {
					f.metrics.OnError(err)
				},
			},
			&nbs_client.DiscoveryClientOpts{
				HardTimeout: discoveryClientHardTimeout,
				SoftTimeout: discoveryClientSoftTimeout,
			},
			NewNbsClientLog(nbs_client.LOG_DEBUG),
		)
		if err != nil {
			return err
		}

		f.clients[zoneID] = &client{
			nbs:     nbs,
			metrics: f.metrics,
		}
	}

	return nil
}

func (f *factory) HasClient(zoneID string) bool {
	_, ok := f.clients[zoneID]
	return ok
}

func (f *factory) GetClient(
	ctx context.Context,
	zoneID string,
) (Client, error) {

	client, ok := f.clients[zoneID]
	if !ok {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf("unknown zoneID %v", zoneID),
		}
	}

	return client, nil
}

func (f *factory) GetClientFromDefaultZone(
	ctx context.Context,
) (Client, error) {

	var zoneID string

	for id := range f.config.GetZones() {
		zoneID = id
		// It's OK to use first known zone.
		break
	}

	return f.GetClient(ctx, zoneID)
}

////////////////////////////////////////////////////////////////////////////////

func CreateFactoryWithCreds(
	ctx context.Context,
	config *nbs_config.ClientConfig,
	creds auth.Credentials,
	metricsRegistry metrics.Registry,
) (Factory, error) {

	if config.GetDisableAuthentication() {
		creds = nil
	}

	f := &factory{
		config:      config,
		credentials: creds,
		metrics:     makeClientMetrics(metricsRegistry),
	}
	err := f.initClients(ctx)
	if err != nil {
		return nil, err
	}

	return f, nil
}

func CreateFactory(
	ctx context.Context,
	config *nbs_config.ClientConfig,
	metricsRegistry metrics.Registry,
) (Factory, error) {

	return CreateFactoryWithCreds(ctx, config, nil, metricsRegistry)
}
