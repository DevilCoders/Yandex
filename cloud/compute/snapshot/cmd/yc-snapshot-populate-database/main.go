package main

import (
	"context"
	"flag"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
)

var (
	erase    = flag.Bool("erase", false, "drop tables")
	noCreate = flag.Bool("no-create", false, "do not create tables")
)

func main() {
	flag.Parse()
	cfg, err := config.GetConfig()
	if err != nil {
		zap.L().Fatal("Unable to read config", zap.Error(err))
	}

	ops := misc.TableOpDBOnly
	if !*noCreate {
		ops |= misc.TableOpCreate
	}
	if *erase {
		ops |= misc.TableOpDrop
	}

	logger := logging.SetupCliLogging()
	ctx := ctxlog.WithLogger(context.Background(), logger)

	globalauth.InitCredentials(ctx, cfg.General.TokenPolicy)

	f, err := lib.NewFacadeOps(ctx, &cfg, ops)
	if err != nil {
		zap.L().Fatal("Failed to create facade", zap.Error(err))
	}
	defer func() { _ = f.Close(ctx) }()

	if err = f.Close(ctx); err != nil {
		zap.L().Fatal("Failed to close facade", zap.Error(err))
	}

}
