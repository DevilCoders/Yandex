package main

import (
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	"a.yandex-team.ru/cloud/billing/go/pkg/ualogs"
	"a.yandex-team.ru/library/go/core/log"
	lz "a.yandex-team.ru/library/go/core/log/zap"
)

func init() {
	_ = ualogs.RegisterSink()
	cfg := lz.JSONConfig(log.InfoLevel)
	cfg.OutputPaths = append(cfg.OutputPaths, "logbroker://localhost:3113")
	logging.SetLogger(lz.Must(cfg))
}

func logCycle() {
	logger := logging.Logger()
	for {
		logger.Info("test message", log.String("key", "value string"))
		time.Sleep(time.Second * 3)
	}
}
