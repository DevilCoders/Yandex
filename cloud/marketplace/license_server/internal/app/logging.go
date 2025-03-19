package app

import (
	"fmt"

	ozap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/cloud/billing/go/pkg/zapjournald"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app/config"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func SetupLogging(c *config.Logger) error {
	if c == nil {
		return fmt.Errorf("logger config is required")
	}

	zapCfg := zap.JSONConfig(c.Level)

	// We use logger from strongly wrapped tooling. Stack traces are not usefull.
	zapCfg.DisableCaller = true
	zapCfg.DisableStacktrace = true

	zapCfg.OutputPaths = c.FilePath

	logger, err := zap.New(zapCfg)
	if err != nil {
		return err
	}

	if c.EnableJournald {
		jdCore := zapjournald.NewCore(zap.ZapifyLevel(c.Level))

		logger.L = logger.L.WithOptions(
			ozap.WrapCore(func(c zapcore.Core) zapcore.Core {
				return zapcore.NewTee(c, jdCore)
			}),
		)
	}

	logging.SetLogger(logger)

	return nil
}
