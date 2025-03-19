package grpc

import (
	"context"
	"fmt"

	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	mlockapi "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	"a.yandex-team.ru/library/go/core/log"
)

type Config struct {
	Host      string                `json:"host" yaml:"host"`
	Transport grpcutil.ClientConfig `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	return Config{
		Transport: grpcutil.DefaultClientConfig(),
	}
}

type GRPCLocker struct {
	mlock mlockapi.LockServiceClient
}

var _ mlockclient.Locker = &GRPCLocker{}

func NewMlockGRPCClient(mlock mlockapi.LockServiceClient) *GRPCLocker {
	return &GRPCLocker{
		mlock: mlock,
	}
}

func NewFromConfig(ctx context.Context, cfg Config, appName string, creds credentials.PerRPCCredentials, l log.Logger) (*GRPCLocker, error) {
	conn, err := grpcutil.NewConn(ctx, cfg.Host, appName, cfg.Transport, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, fmt.Errorf("failed to initialize grpc connection %w", err)
	}

	return NewMlockGRPCClient(mlockapi.NewLockServiceClient(conn)), nil
}
