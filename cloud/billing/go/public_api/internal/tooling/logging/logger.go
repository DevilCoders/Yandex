package logging

import (
	"a.yandex-team.ru/cloud/billing/go/pkg/zapjournald"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/config"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func InitLogger(appCfg *config.Config) (*zap.Logger, error) {

	if appCfg.EnableJournald {
		jdCore := zapjournald.NewCore(zap.ZapifyLevel(log.InfoLevel))
		logger := zap.NewWithCore(jdCore)
		return logger, nil
	}

	zapCfg := zap.JSONConfig(log.InfoLevel)
	zapCfg.DisableCaller = true
	zapCfg.DisableStacktrace = true

	return zap.New(zapCfg)
}
