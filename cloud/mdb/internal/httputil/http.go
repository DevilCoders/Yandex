package httputil

import (
	"context"
	"net"
	"net/http"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ServeConfig struct {
	Addr            string        `yaml:"addr,omitempty"`
	ShutdownTimeout time.Duration `yaml:"shutdown_timeout,omitempty"`
	Logging         LoggingConfig `yaml:"logging,omitempty"`
}

func DefaultServeConfig() ServeConfig {
	return ServeConfig{
		ShutdownTimeout: time.Second * 1,
	}
}

// Serve http server in separate goroutine
func Serve(srv *http.Server, l log.Logger) error {
	addr := srv.Addr
	if addr == "" {
		addr = ":http"
	}

	listener, err := net.Listen("tcp", addr)
	if err != nil {
		return xerrors.Errorf("http server listen at %q:: %w", srv.Addr, err)
	}

	go func() {
		l.Info("serving http", log.String("addr", srv.Addr))
		if err := srv.Serve(listener); err != nil && err != http.ErrServerClosed {
			l.Error("error while serving http", log.String("addr", srv.Addr), log.Error(err))
		}
	}()

	return nil
}

// WaitForShutdown of http server when context is done
func WaitForShutdown(ctx context.Context, srv *http.Server, timeout time.Duration) error {
	<-ctx.Done()
	return Shutdown(srv, timeout)
}

// Shutdown http server
func Shutdown(srv *http.Server, timeout time.Duration) error {
	shutdownCtx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()
	if err := srv.Shutdown(shutdownCtx); err != nil {
		return xerrors.Errorf("error while shutting down http server %q: %w", srv.Addr, err)
	}

	return nil
}
