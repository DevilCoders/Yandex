package grpcutil

import (
	"context"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/library/go/certifi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// PerRPCCredentialsStatic provides static credentials for authentication
type PerRPCCredentialsStatic struct {
	RequestMetadata map[string]string
	Security        bool
}

var _ credentials.PerRPCCredentials = &PerRPCCredentialsStatic{}

func (s *PerRPCCredentialsStatic) GetRequestMetadata(_ context.Context, _ ...string) (map[string]string, error) {
	return s.RequestMetadata, nil
}

func (s *PerRPCCredentialsStatic) RequireTransportSecurity() bool {
	return s.Security
}

type ClientConfig struct {
	Security SecurityConfig                 `yaml:"security"`
	Retries  interceptors.ClientRetryConfig `yaml:"retries"`
	Logging  interceptors.LoggingConfig     `yaml:"logging,omitempty"`
}

func DefaultClientConfig() ClientConfig {
	return ClientConfig{
		Retries: interceptors.DefaultClientRetryConfig(),
		Logging: interceptors.DefaultLoggingConfig(),
	}
}

type ClientOption func(opts []grpc.DialOption) []grpc.DialOption

func WithClientCredentials(creds credentials.PerRPCCredentials) ClientOption {
	return func(opts []grpc.DialOption) []grpc.DialOption {
		return append(opts, grpc.WithPerRPCCredentials(creds))
	}
}

func WithClientKeepalive(kp keepalive.ClientParameters) ClientOption {
	return func(opts []grpc.DialOption) []grpc.DialOption {
		return append(opts, grpc.WithKeepaliveParams(kp))
	}
}

// NewConn constructs new gRPC client connection with predefined options that are considered adequate
func NewConn(ctx context.Context, target, userAgent string, cfg ClientConfig, l log.Logger, opts ...ClientOption) (*grpc.ClientConn, error) {
	// Set some sane defaults
	if cfg.Retries.MaxTries > 1 && cfg.Retries.PerRetryTimeout == 0 {
		cfg.Retries.PerRetryTimeout = 2 * time.Second
	}

	dialOpts := []grpc.DialOption{
		grpc.WithUnaryInterceptor(interceptors.ChainUnaryClientInterceptors(cfg.Retries, cfg.Logging, l)),
		grpc.WithStreamInterceptor(interceptors.ChainStreamClientInterceptors(l)),
		grpc.WithUserAgent(userAgent),
		grpc.WithKeepaliveParams(
			// Safe values that work well with default keepalive settings
			keepalive.ClientParameters{
				Time:    6 * time.Minute,
				Timeout: time.Second,
			}),
	}

	for _, opt := range opts {
		dialOpts = opt(dialOpts)
	}

	var err error
	dialOpts, err = applyTransportCredentials(cfg.Security, dialOpts)
	if err != nil {
		return nil, err
	}

	return grpc.DialContext(ctx, target, dialOpts...)
}

type SecurityConfig struct {
	TLS      TLSConfig `yaml:"tls"`
	Insecure bool      `yaml:"insecure,omitempty"`
}

type TLSConfig struct {
	CAFile     string `yaml:"ca_file"`
	ServerName string `yaml:"server_name,omitempty"`
}

func applyTransportCredentials(cfg SecurityConfig, opts []grpc.DialOption) ([]grpc.DialOption, error) {
	if cfg.Insecure {
		opts = append(opts, grpc.WithInsecure())
		return opts, nil
	}

	var tc credentials.TransportCredentials
	if cfg.TLS.CAFile == "" {
		pool, err := certifi.NewCertPoolSystem()
		if err != nil {
			return nil, xerrors.Errorf("failed to get system cert pool: %w", err)
		}

		tc = credentials.NewClientTLSFromCert(pool, cfg.TLS.ServerName)
	} else {
		var err error
		tc, err = credentials.NewClientTLSFromFile(cfg.TLS.CAFile, cfg.TLS.ServerName)
		if err != nil {
			return nil, xerrors.Errorf("failed to create transport credentials from tls config %+v: %w", cfg, err)
		}
	}

	opts = append(opts, grpc.WithTransportCredentials(tc))
	return opts, nil
}
