package server

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"os"
	"os/signal"
	"runtime"
	"syscall"
	"time"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/mon"
	"a.yandex-team.ru/cloud/compute/go-common/util"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/activation"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
)

var (
	// SignalListener is a channel for interruption signals.
	// Importing this package automatically installs signal handler.
	SignalListener = make(chan os.Signal, 1)
)

func init() {
	signal.Notify(SignalListener, os.Interrupt, syscall.SIGTERM)
	util.IgnoreSigpipe()

	runtime.SetBlockProfileRate(10000)
}

func startServers(ctx context.Context, s *Server, ss *sockets) error {
	defer ss.Close()
	if l := ss.debugListener; l != nil {
		log.G(ctx).Info("start debug server", zap.Stringer("endpoint", l.Addr()))
		StartMetricsServer(ctx, l)
	}

	log.G(ctx).Info("start GRPC server", zap.Stringer("endpoint", ss.grpcListener.Addr()))
	log.G(ctx).Info("start HTTP server", zap.Stringer("endpoint", ss.httpListener.Addr()))
	err := s.Start(Listeners{
		GRPCListener: ss.grpcListener,
		HTTPListener: ss.httpListener,
	})
	return err
}

type sockets struct {
	grpcListener, httpListener, debugListener net.Listener
}

func (s *sockets) Close() {
	if s.grpcListener != nil {
		if err := s.grpcListener.Close(); err != nil {
			zap.L().Warn("grpcListener.Close failed", zap.Error(err))
		}
	}
	if s.httpListener != nil {
		if err := s.httpListener.Close(); err != nil {
			zap.L().Warn("httpListener.Close failed", zap.Error(err))
		}
	}
	if s.debugListener != nil {
		if err := s.debugListener.Close(); err != nil {
			zap.L().Warn("debugListener.Close failed", zap.Error(err))
		}
	}
}

// createListeners returns all listened socket for the current process
// there're 2 possible ways:
// * sockets are provided by systemd or other one via fds
// * we bind sockets on addresses specified in the configuration
// In case of Systemd both socks for GRPC and HTTP must be provided in the same time
// activation sockets map:
// 1. GRPC listener
// 2. HTTP listener
// 3. Debug HTTP listener (optional)
func createListeners(cfg config.Config) (*sockets, error) {
	listeners, err := activation.Listeners()
	if err != nil {
		return nil, fmt.Errorf("get listeners failed: %v", err)
	}

	closeLs := func(ls []net.Listener) {
		for _, l := range ls {
			if err := l.Close(); err != nil {
				zap.L().Warn("listener.Close failed", zap.Error(err))
			}
		}
	}

	l := len(listeners)
	switch {
	case l == 0:
		if cfg.Server.GRPCEndpoint == nil {
			return nil, fmt.Errorf("grpcEndpoint is not specified")
		}
		if cfg.Server.HTTPEndpoint == nil {
			return nil, fmt.Errorf("httpEndpoint is not specified")
		}

		var (
			err error
			ss  = new(sockets)
		)
		ss.grpcListener, err = net.Listen(cfg.Server.GRPCEndpoint.Network, cfg.Server.GRPCEndpoint.Addr)
		if err != nil {
			// NOTE: just in case for possible future modifications
			ss.Close()
			return nil, fmt.Errorf("failed to listen on gRPC endpoint %s: %v", cfg.Server.GRPCEndpoint, err)
		}
		ss.httpListener, err = net.Listen(cfg.Server.HTTPEndpoint.Network, cfg.Server.HTTPEndpoint.Addr)
		if err != nil {
			ss.Close()
			return nil, fmt.Errorf("failed to listen on HTTP endpoint %s: %v", cfg.Server.HTTPEndpoint, err)
		}
		if debugEndp := cfg.DebugServer.HTTPEndpoint; debugEndp != nil {
			ss.debugListener, err = net.Listen(debugEndp.Network, debugEndp.Addr)
			if err != nil {
				ss.Close()
				return nil, fmt.Errorf("failed to listen on Debug HTTP endpoint %s: %v", debugEndp, err)
			}
		}
		return ss, nil
	case l == 1:
		closeLs(listeners)
		return nil, fmt.Errorf("only 1 socket provided by Systemd or etc, need 2 or 3")
	case l >= 2 && l < 4:
		ss := &sockets{
			grpcListener: listeners[0],
			httpListener: listeners[1],
		}
		if len(listeners) == 3 {
			ss.debugListener = listeners[2]
		}
		return ss, nil
	default:
		closeLs(listeners)
		return nil, fmt.Errorf("at least 2 sockets not 1 must be passed by Systemd or etc")
	}
}

// Start starts GRPC, HTTP and debug servers.
// Installs signal handlers and monitoring checks.
func Start(ctx context.Context, cfg config.Config) error {
	tracer, err := tracing.InitJaegerTracing(cfg.Tracing.Config)
	if err != nil {
		log.G(ctx).Error("Can't start tracer", zap.Error(err))
		return err
	}
	defer func() { log.InfoErrorCtx(ctx, tracer.Close(), "Close tracer") }()

	srv, err := NewServer(ctx, &cfg)
	if err != nil {
		log.G(ctx).Error("Failed to start server", zap.Error(err))
		return err
	}
	defer srv.Close(ctx)

	// set monitoring handler
	if err := setupMonitoring(ctx, srv, cfg); err != nil {
		log.G(ctx).Error("Failed to register monitoring checks", zap.Error(err))
		return err
	}

	ss, err := createListeners(cfg)
	if err != nil {
		log.G(ctx).Error("failed to create or inherit sockets", zap.Error(err))
		return err
	}
	defer ss.Close()

	go func() {
		// NOTE: we need timeout for GracefulStop. Although grpc.Server does not
		// support the same Shutdown method with Context as http.Server since 1.8
		// we can emulate this behavior with an extra goroutine.
		// The only guarantee need - thread safe Shutdown and Stop methods
		s := <-SignalListener
		shutdownTimeout := 30 * time.Second
		log.G(ctx).Info("initiate graceful stop after receiving a signal", zap.Stringer("signal", s), zap.Duration("timeout", shutdownTimeout))

		var onSuccefullyShutdowned = make(chan struct{})
		shutdownCtx, cf := context.WithTimeout(ctx, shutdownTimeout)
		defer cf()
		go func() {
			// Gracefully stop the server
			srv.GracefulStop(shutdownCtx)
			close(onSuccefullyShutdowned)
		}()
		select {
		case <-shutdownCtx.Done():
			log.G(ctx).Warn("shutdown timeout has expired. Stop the server immediately.")
			srv.Stop()
		case <-onSuccefullyShutdowned:
			// pass
		case s = <-SignalListener:
			// To allow forced stop with SIGINT, SIGTERM before expiration of shutdownTimeout
			log.G(ctx).Warn("signal arrives to force non-graceful stop", zap.Stringer("signal", s))
			srv.Stop()
		}

		// Stop debug server: we don't care if it's not gracefully stopped
		if err = StopMetricsServer(); err != nil {
			log.G(ctx).Warn("debugHTTPServer.Close failed", zap.Error(err))
		}
	}()
	if err = startServers(ctx, srv, ss); err != nil {
		log.G(ctx).Error("failed to start servers", zap.Error(err))
		return err
	}
	return nil
}

func setupMonitoring(ctx context.Context, srv *Server, cfg config.Config) error {
	repo := mon.NewRepository("snapshot", cfg.Monitoring.RepositoryOptions()...)
	if err := srv.RegisterMon(repo); err != nil {
		log.G(ctx).Fatal("Failed to register monitoring checks", zap.Error(err))
		return err
	}
	metricsServerMux.Handle("/status", repo)
	return nil
}

// StartMetricsServer starts only debug server.
func StartMetricsServer(ctx context.Context, l net.Listener) {
	go func() {
		if err := metricsServer.Serve(l); err != nil && err != http.ErrServerClosed {
			log.G(ctx).Error("debug server start failed", zap.Error(err))
		}
	}()
}

// StopMetricsServer stops debug server.
func StopMetricsServer() error {
	return metricsServer.Close()
}
