package logging

import (
	"sync"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func SetLogger(l log.Structured) {
	defLogger.mu.Lock()
	defer defLogger.mu.Unlock()

	defLogger.l = l
}

func Logger() log.Structured {
	return defLogger.get()
}

func LibsLogger(noSkips bool) log.Logger {
	if noSkips {
		return Logger().Logger()
	}
	return &constantLogger{logger: Logger().Logger()}
}

var defLogger logger

type logger struct {
	l log.Structured

	mu sync.RWMutex
}

func (t *logger) get() log.Structured {
	t.mu.RLock()
	defer t.mu.RUnlock()
	if t.l != nil {
		return t.l
	}
	return &nop.Logger{}
}

type constantLogger struct {
	logger log.Logger
}

func (l *constantLogger) With(fields ...log.Field) log.Logger {
	return &constantLogger{
		logger: log.With(l.logger, fields...),
	}
}

func (l *constantLogger) Warn(msg string, fields ...log.Field)  { l.logger.Warn(msg, fields...) }
func (l *constantLogger) Error(msg string, fields ...log.Field) { l.logger.Error(msg, fields...) }
func (l *constantLogger) Fatal(msg string, fields ...log.Field) { l.logger.Fatal(msg, fields...) }

func (l *constantLogger) Warnf(format string, args ...interface{})  { l.logger.Warnf(format, args...) }
func (l *constantLogger) Errorf(format string, args ...interface{}) { l.logger.Errorf(format, args...) }
func (l *constantLogger) Fatalf(format string, args ...interface{}) { l.logger.Fatalf(format, args...) }

func (l *constantLogger) Trace(msg string, fields ...log.Field) {}
func (l *constantLogger) Debug(msg string, fields ...log.Field) {}
func (l *constantLogger) Info(msg string, fields ...log.Field)  {}

func (l *constantLogger) Tracef(format string, args ...interface{}) {}
func (l *constantLogger) Debugf(format string, args ...interface{}) {}
func (l *constantLogger) Infof(format string, args ...interface{})  {}

func (l *constantLogger) Fmt() log.Fmt                 { return l }
func (l *constantLogger) Structured() log.Structured   { return l }
func (l *constantLogger) Logger() log.Logger           { return l }
func (l *constantLogger) WithName(_ string) log.Logger { return l }
