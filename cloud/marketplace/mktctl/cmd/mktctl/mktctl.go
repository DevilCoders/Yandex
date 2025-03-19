//
//	ToDo:
//		* jwt token auth (make browser dependency optional)
//
package main

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/commands"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func main() {
	lgr, logLevel := logger.New()
	ctx := logger.NewContext(context.Background(), lgr)

	cfg := config.New()
	ctx = config.NewContext(ctx, cfg)

	init := func() {
		var zapLevel zapcore.Level
		if err := zapLevel.UnmarshalText([]byte(cfg.LogLevel)); err != nil {
			logger.FatalCtx(ctx, "parse log-level", zap.Error(err))
		}
		logLevel.SetLevel(zapLevel)

		if err := cfg.Load(); err != nil {
			logger.FatalCtx(ctx, "load config", zap.Error(err))
		}
	}
	cobra.OnInitialize(init)

	if err := commands.Execute(ctx); err != nil {
		logger.FatalCtx(ctx, "execute command", zap.Error(err))
	}
}
