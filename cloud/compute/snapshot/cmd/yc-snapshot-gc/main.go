package main

import (
	"context"
	"flag"
	"net"
	"os"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/server"
)

var (
	oneShot = flag.Bool("oneshot", false, "do one loop and exit")
)

func startDebugServer(ctx context.Context, cfg config.Config) {
	if debugEndp := cfg.DebugServer.GCEndpoint; debugEndp != nil {
		l, err := net.Listen(debugEndp.Network, debugEndp.Addr)
		if err != nil {
			return
		}

		log.G(ctx).Info("start debug server", zap.Stringer("endpoint", l.Addr()))
		server.StartMetricsServer(ctx, l)
	}
}

func main() {
	flag.Parse()
	cfg, err := config.GetConfig()
	if err != nil {
		zap.L().Fatal("Unable to read config", zap.Error(err))
	}

	logger, closeFunc, err := logging.SetupLogging(cfg)
	if err != nil {
		zap.L().Fatal("failed to setup logging", zap.Error(err))
	}
	defer closeFunc()

	ctx := log.WithLogger(context.Background(), logger)

	gc, err := lib.NewGC(ctx, &cfg)
	if err != nil {
		log.G(ctx).Fatal("failed to create GC", zap.Error(err))
	}
	defer gc.Close()

	ctx, cancel := context.WithCancel(ctx)
	go func() {
		<-server.SignalListener
		log.G(ctx).Info("Interrupt signal received")
		cancel()
		gc.Stop()
	}()

	startDebugServer(ctx, cfg)
	//nolint:errcheck
	defer server.StopMetricsServer()

	if *oneShot {
		err = gc.Once(ctx)
	} else {
		err = gc.Run(ctx)
	}

	switch err {
	case nil:
	case misc.ErrNothingProcessed:
	default:
		log.G(ctx).Error("failed to run", zap.Error(err))
		os.Exit(1)
	}
}
