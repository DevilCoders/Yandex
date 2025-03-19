package app

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app/config"
	grpc_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

func makeGRPCServiceConfig(in *config.GRPCService) []grpc_service.Option {
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
	}

}

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

func makeMarketplaceClientConfig(in *config.MarketplaceClient) (out *marketplace.Config) {
	if in == nil {
		logging.Logger().Warn("marketplace private api client config is required")
		return
	}

	out = &marketplace.Config{
		Endpoint:       in.Endpoint,
		OpTimeout:      in.OpTimeout,
		OpPollInterval: in.OpPollInterval,
	}

	return
}

func makeAutorecreateWorkerServiceConfig(in *config.WorkerService) error {
	if in == nil {
		logging.Logger().Warn("grpc service config is required")
		return nil
	}

	return nil
}

func MakeYDBConfig(in *config.YDB) (out []ydb.Option, err error) {
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
