package logging

import (
	"a.yandex-team.ru/library/go/core/log/zap"
)

////////////////////////////////////////////////////////////////////////////////

func CreateStderrLogger(level Level) Logger {
	return zap.Must(zap.ConsoleConfig(level))
}
