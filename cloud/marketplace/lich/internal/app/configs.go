package app

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/access-backend"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/app/config"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/metrics"

	rm "a.yandex-team.ru/cloud/marketplace/pkg/clients/resource-manager"
	monitoring "a.yandex-team.ru/cloud/marketplace/pkg/monitoring/http"

	grpc_service "a.yandex-team.ru/cloud/marketplace/lich/internal/services/grpc"
	http_service "a.yandex-team.ru/cloud/marketplace/lich/internal/services/http"
)

func makeBillingClientConfig(in *config.BillingClient) (out *billing.Config) {
	if in == nil {
		logging.Logger().Warn("billing private api client config is required")
		return
	}

	out = &billing.Config{
		Endpoint:   in.Endpoint,
		RetryCount: in.RetryCount,
	}

	return
}

func makeHTTPServiceConfig(in *config.HTTPService) (out *http_service.Config) {
	if in == nil {
		logging.Logger().Warn("no http service config provided")
		return
	}

	out = &http_service.Config{
		ListenEndpoint: in.ListenEndpoint,
	}

	return
}

func makeGRPCServiceConfig(in *config.GRPCService, metricsHub *metrics.Hub) []grpc_service.Option {
	if in == nil {
		logging.Logger().Warn("grpc service config is required")
		return nil
	}

	return []grpc_service.Option{
		grpc_service.WithListenEndpoint(in.ListenEndpoint),
		grpc_service.WithAccessLogger(
			logging.Named("grpc.access"),
		),
		grpc_service.WithLogger(
			logging.Named("grpc"),
		),
		grpc_service.WithMetricsHub(metricsHub),
	}
}

func makeResourceManagerConfig(in *config.ResourceManager) (out *rm.Config) {
	if in == nil {
		logging.Logger().Warn("resource-manager service config is provided")
		return
	}

	out = &rm.DefaultConfig

	out.Endpoint = in.Endpoint
	out.CAPath = in.CAPath

	return
}

func makeYDBConfig(in *config.YDB) (out []ydb.Option, err error) {
	if in == nil {
		logging.Logger().Warn("ydb config is required")
		return
	}

	tls, err := tlsFromPath(in.CAPath)
	if err != nil {
		return nil, err
	}

	out = append(out,
		ydb.WithEndpoint(in.Endpoint),
		ydb.WithDBRoot(in.DBRoot),
		ydb.WithDatabase(in.Database),
		ydb.WithTLS(tls),
	)

	return
}

func makeMonitoringConfig(in *config.Monitoring) (out *monitoring.Config) {
	if in == nil {
		logging.Logger().Info("monitoring config is absent")
		return
	}

	out = &monitoring.Config{
		ListenEndpoint: in.ListenEndpoint,
	}

	return out
}

func makeAccessServiceConfig(in *config.AccessService) (out *access.Config) {
	if in == nil {
		logging.Logger().Warn("access service config is required")
		return
	}

	out = &access.Config{
		Endpoint: in.Endpoint,
		CAPath:   in.CAPath,
	}

	return
}

func tlsFromPath(CAPath string) (*tls.Config, error) {
	if CAPath == "" {
		return nil, nil
	}

	p, err := ioutil.ReadFile(CAPath)
	if err != nil {
		return nil, err
	}

	roots, err := x509.SystemCertPool()
	if err != nil {
		return nil, err
	}

	if ok := roots.AppendCertsFromPEM(p); !ok {
		return nil, fmt.Errorf("parse pem error")
	}

	t := &tls.Config{
		MinVersion: tls.VersionTLS12,
		RootCAs:    roots,
	}

	return t, nil
}

func makeTracingConfig(serviceName string, in *config.Tracer) *tracing.Config {
	if in == nil {
		return nil
	}

	return &tracing.Config{
		ServiceName: serviceName,

		LocalAgentHostPort:  in.LocalAgentHostPort,
		BufferFlushInterval: in.BufferFlushInterval,
		QueueSize:           in.QueueSize,
	}
}
