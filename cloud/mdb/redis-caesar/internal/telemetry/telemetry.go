package telemetry

//go:generate $GOPATH/bin/mockgen -source ./telemetry.go -destination ./mock/telemetry.go -package mock

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"net/http/pprof"

	"a.yandex-team.ru/cloud/mdb/redis-caesar/pkg/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

// StartTelemetryServer starts web-server with pprof endpoints.
func StartTelemetryServer(ctx context.Context, conf *config.TelemetryConfig) (func(context.Context) error, error) {
	listener, err := net.Listen("tcp", conf.Address)
	if err != nil {
		return nil, fmt.Errorf("unable to open TCP port: %w", err)
	}
	serverMux := http.NewServeMux()
	if conf.Profiling {
		serverMux.HandleFunc("/pprof/", pprof.Index)
		serverMux.HandleFunc("/pprof/cmdline", pprof.Cmdline)
		serverMux.HandleFunc("/pprof/profile", pprof.Profile)
		serverMux.HandleFunc("/pprof/symbol", pprof.Symbol)
		serverMux.HandleFunc("/pprof/trace", pprof.Trace)
		serverMux.HandleFunc("/pprof/heap", pprof.Handler("heap").ServeHTTP)
		serverMux.HandleFunc("/pprof/goroutine", pprof.Handler("goroutine").ServeHTTP)
	}
	server := &http.Server{Handler: serverMux}
	go func() {
		err := server.Serve(listener)
		if err != nil {
			panic(fmt.Sprintf("unable to start telemetry server on %s: %s", conf.Address, err.Error()))
		}
	}()

	stopServer := func(ctx context.Context) error {
		if err := server.Shutdown(ctx); err != nil {
			return fmt.Errorf("can't stop telemetry server correctly: %w", err)
		}

		return nil
	}

	return stopServer, nil
}

func ConfigureLogging(ctx context.Context, conf *config.TelemetryConfig) (log.Logger, error) {
	logLevelN, err := log.ParseLevel(conf.LogLevel)
	if err != nil {
		return nil, fmt.Errorf("unable to configure logger: %w", err)
	}
	loggerConfig := zap.ConsoleConfig(logLevelN)
	logger, err := zap.New(loggerConfig)
	if err != nil {
		return nil, fmt.Errorf("unable to configure logger: %w", err)
	}

	return logger, nil
}
