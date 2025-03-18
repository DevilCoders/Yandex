package logger

import (
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

var Log log.Logger = new(nop.Logger)

func Setup(lvl log.Level) {
	Log = zap.Must(zap.ConsoleConfig(lvl))
}
