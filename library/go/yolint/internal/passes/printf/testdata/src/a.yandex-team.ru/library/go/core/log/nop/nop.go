package nop

import (
	"os"

	"a.yandex-team.ru/library/go/core/log"
)

// Logger that does nothing
type Logger struct{}

// Logger returns general logger
func (l *Logger) Logger() log.Logger {
	return &Logger{}
}

// Tracef implements Tracef method of log.Logger interface
func (l *Logger) Tracef(format string, args ...interface{}) {}

// Debugf implements Debugf method of log.Logger interface
func (l *Logger) Debugf(format string, args ...interface{}) {}

// Infof implements Infof method of log.Logger interface
func (l *Logger) Infof(format string, args ...interface{}) {}

// Warnf implements Warnf method of log.Logger interface
func (l *Logger) Warnf(format string, args ...interface{}) {}

// Errorf implements Errorf method of log.Logger interface
func (l *Logger) Errorf(format string, args ...interface{}) {}

// Fatalf implements Fatalf method of log.Logger interface
func (l *Logger) Fatalf(format string, args ...interface{}) {
	os.Exit(1)
}
