package main

import (
	"context"
	"expvar"
	"flag"
	"fmt"

	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"

	"go.uber.org/zap"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/pkg/version"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/server"
)

// TODO: move all metrics out to a package
var (
	fmtVersion = fmt.Sprintf("version=%s build=%s hash=%s tag=%s", version.Version, version.Build, version.GitHash, version.GitTag)

	showVersion = flag.Bool("version", false, "show version and exit")
)

func init() {
	expvar.NewString("version").Set(fmtVersion)
}

func main() {
	flag.Parse()
	if *showVersion {
		fmt.Println(fmtVersion)
		return
	}

	cfg, err := config.GetConfig()
	if err != nil {
		zap.L().Fatal("Unable to read config", zap.Error(err))
	}

	logger, closeFunc, err := logging.SetupLogging(cfg)
	if err != nil {
		zap.L().Fatal("Failed to setup logging", zap.Error(err))
	}
	defer closeFunc()

	dockerprocess.SetDockerConfig(cfg.Nbd.DockerConfig)

	ctx := log.WithLogger(context.Background(), logger)
	if err := server.Start(ctx, cfg); err != nil {
		log.G(ctx).Fatal("Failed to start server", zap.Error(err))
	}

}
