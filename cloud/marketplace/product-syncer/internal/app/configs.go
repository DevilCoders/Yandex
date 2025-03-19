package app

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io/ioutil"
	"time"

	"a.yandex-team.ru/cloud/marketplace/pkg/clients/compute"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/operation"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/app/config"
)

const (
	DefaultMktOpTimeout          = "5m"
	DefaultMktOpPollInterval     = "5s"
	DefaultComputeOpTimeout      = "5m"
	DefaultComputeOpPollInterval = "5s"
)

func makeMarketplaceClientConfig(in *config.MarketplaceClient) (out marketplace.Config, err error) {
	if in == nil {
		err = fmt.Errorf("marketplace private api client config is required")
		return
	}

	out.Endpoint = in.Endpoint

	// because confita can't work with 'required' fields
	interval := in.OpPollInterval
	if interval == "" {
		interval = DefaultMktOpPollInterval
	}
	timeout := in.OpTimeout
	if in.OpTimeout == "" {
		timeout = DefaultMktOpTimeout
	}

	out.OpPollInterval, err = time.ParseDuration(interval)
	if err != nil {
		err = fmt.Errorf("failed to parse operation-poll-interval parameter: %w", err)
		return
	}

	out.OpTimeout, err = time.ParseDuration(timeout)
	if err != nil {
		err = fmt.Errorf("failed to parse operation-timeout parameter: %w", err)
		return
	}

	return
}

func makeComputeClientConfig(in *config.YcClient) (out *compute.Config) {
	if in == nil {
		logging.Logger().Warn("yc api client config is required")
		return
	}

	out = &compute.Config{
		Endpoint:    in.ComputeEndpoint,
		DebugMode:   false,
		CAPath:      in.CAPath,
		InitTimeout: 20 * time.Second,
	}

	return
}

func makeOperationConfig(in *config.YcClient) (out *operation.Config, err error) {
	if in == nil {
		err = fmt.Errorf("yc api client config is required")
		return
	}

	out = &operation.Config{
		Endpoint:    in.OperationEndpoint,
		DebugMode:   false,
		CAPath:      in.CAPath,
		InitTimeout: 20 * time.Second,
	}

	// because confita can't work with 'required' fields
	interval := in.OpPollInterval
	if interval == "" {
		interval = DefaultComputeOpPollInterval
	}
	timeout := in.OpTimeout
	if in.OpTimeout == "" {
		timeout = DefaultComputeOpTimeout
	}

	out.OpPollInterval, err = time.ParseDuration(interval)
	if err != nil {
		err = fmt.Errorf("failed to parse operation-poll-interval parameter: %w", err)
		return
	}

	out.OpTimeout, err = time.ParseDuration(timeout)
	if err != nil {
		err = fmt.Errorf("failed to parse operation-timeout parameter: %w", err)
		return
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
