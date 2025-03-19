package logger

import (
	stdLog "log"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"

	server_log "github.com/fclairamb/ftpserver/server/log"
)

var ftpLogger log.Logger = &nop.Logger{}
var level log.Level = log.FatalLevel

type FtpdLogger struct {
	logger log.Logger
}

func (l *FtpdLogger) Debug(v ...interface{}) { l.logger.Debugf("%v", v) }
func (l *FtpdLogger) Info(v ...interface{})  { l.logger.Debugf("%v", v) }
func (l *FtpdLogger) Warn(v ...interface{})  { l.logger.Debugf("%v", v) }
func (l *FtpdLogger) Error(v ...interface{}) { l.logger.Debugf("%v", v) }

func (l *FtpdLogger) With(v ...interface{}) server_log.Logger {
	return &FtpdLogger{
		logger: log.With(l.logger, log.Any("with", v)),
	}
}

func NewFtpdLogger() server_log.Logger {
	return &FtpdLogger{
		logger: ftpLogger,
	}
}

func IsDebug() bool {
	return level <= log.DebugLevel
}

func Get() log.Logger {
	return ftpLogger
}

func Set(levelString string) log.Logger {
	var err error

	if levelString != "" {
		level, err = log.ParseLevel(levelString)
		if err != nil {
			stdLog.Fatalf("%v", err)
		}
		ftpLogger, err = zap.NewDeployLogger(level)
		if err != nil {
			stdLog.Fatalf("%v", err)
		}

	}
	return ftpLogger
}
