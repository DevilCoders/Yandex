package scope

import (
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
	"a.yandex-team.ru/library/go/core/log"
)

type globalScope struct {
	logger log.Logger
}

func (gs *globalScope) Logger() log.Logger {
	return gs.logger
}

func StartGlobal(logger log.Logger) {
	globalScopeInstance = &globalScope{logger: logger}

	metrics.AppAlive.WithLabelValues().Inc()
}

func FinishGlobal() {
	globalScopeInstance = nil

	metrics.AppAlive.Reset()
}
