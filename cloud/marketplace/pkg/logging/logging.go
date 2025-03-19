package logging

import (
	"sync/atomic"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

var (
	defLogger logger
	nopLogger = &nop.Logger{}
)

type logger struct {
	syncLog atomic.Value
}

func SetLogger(logger log.Structured) {
	defLogger.syncLog.Store(logger)
}

func Logger() log.Structured {
	if rawLog := defLogger.syncLog.Load(); rawLog != nil {
		return rawLog.(log.Structured)
	}

	return nopLogger
}

func LoggerWith(fields ...log.Field) log.Logger {
	structured := Logger()
	return log.With(structured.Logger(), fields...)
}

func Named(name string, fields ...log.Field) log.Logger {
	structured := Logger()
	named := structured.Logger().WithName(name)

	return log.With(named, fields...)
}
