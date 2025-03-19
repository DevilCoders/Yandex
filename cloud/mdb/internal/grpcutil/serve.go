package grpcutil

import (
	"context"
	"io/fs"
	"net"
	"os"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/keepalive"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/internal/address"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ServeConfig struct {
	Addr            string                     `yaml:"addr,omitempty"`
	ShutdownTimeout time.Duration              `yaml:"shutdown_timeout,omitempty"`
	Logging         interceptors.LoggingConfig `yaml:"logging,omitempty"`
}

func DefaultServeConfig() ServeConfig {
	return ServeConfig{
		ShutdownTimeout: time.Second,
		Logging:         interceptors.DefaultLoggingConfig(),
	}
}

func DefaultKeepaliveServerOptions() []grpc.ServerOption {
	return []grpc.ServerOption{
		grpc.KeepaliveEnforcementPolicy(
			keepalive.EnforcementPolicy{
				MinTime:             10 * time.Second,
				PermitWithoutStream: true,
			},
		),
		grpc.KeepaliveParams(
			keepalive.ServerParameters{
				Timeout: 5 * time.Second,
			},
		),
	}
}

type ServeOptions struct {
	UnixSocketMode fs.FileMode
}
type ServeOption func(*ServeOptions)

/*
WithUnixSocketMode change socket mode after it created.
From man unix:

When  creating  a new socket, the owner and group of the socket file are set according to the usual rules.
The socket file has all permissions enabled, other than those that are turned off by the process umask(2).
*/
func WithUnixSocketMode(mode fs.FileMode) ServeOption {
	return func(options *ServeOptions) {
		options.UnixSocketMode = mode
	}
}

// Serve gRPC server in separate goroutine
func Serve(srv *grpc.Server, addr string, l log.Logger, opts ...ServeOption) (net.Listener, error) {
	serveOpts := &ServeOptions{}
	for _, optSetter := range opts {
		optSetter(serveOpts)
	}

	serverAddr, err := address.Parse(addr)
	if err != nil {
		return nil, xerrors.Errorf("parsing address: %w", err)
	}

	if serverAddr.Network == address.Unix {
		if _, err := os.Stat(serverAddr.Address); err == nil {
			l.Warnf("socket '%s' already exists. Probably unclean shutdown. Try to remove it", serverAddr.Address)
			if err := os.Remove(serverAddr.Address); err != nil {
				l.Errorf("error removing old socket %q: %s", serverAddr.Address, err)
			}
		}
	}

	listener, err := net.Listen(serverAddr.Network, serverAddr.Address)
	if err != nil {
		return nil, xerrors.Errorf("gRPC server listen at %q: %w", addr, err)
	}

	if serverAddr.Network == address.Unix && serveOpts.UnixSocketMode != 0 {
		if err := os.Chmod(serverAddr.Address, serveOpts.UnixSocketMode); err != nil {
			return nil, xerrors.Errorf("chmod %s to %s: %w", serverAddr.Address, serveOpts.UnixSocketMode, err)
		}
	}

	go func() {
		l.Info("serving gRPC", log.String("addr", addr))
		if err := srv.Serve(listener); err != nil {
			l.Error("error while serving gRPC", log.String("addr", addr), log.Error(err))
		}
	}()

	return listener, nil
}

// Shutdown gRPC server
func Shutdown(srv *grpc.Server, timeout time.Duration) error {
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	stop := make(chan struct{})
	go func() {
		srv.GracefulStop()
		close(stop)
	}()

	select {
	case <-ctx.Done():
		return xerrors.New("timed out while waiting for gRPC server shutdown")
	case <-stop:
	}

	return nil
}
