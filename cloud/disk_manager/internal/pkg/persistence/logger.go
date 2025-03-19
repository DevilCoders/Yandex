package persistence

import (
	"context"

	"github.com/ydb-platform/ydb-go-sdk/v3/log"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

type logger struct {
	ctx context.Context
}

func (l *logger) Tracef(format string, args ...interface{}) {
	logging.Trace(l.ctx, format, args...)
}

func (l *logger) Debugf(format string, args ...interface{}) {
	logging.Debug(l.ctx, format, args...)
}

func (l *logger) Infof(format string, args ...interface{}) {
	logging.Info(l.ctx, format, args...)
}

func (l *logger) Warnf(format string, args ...interface{}) {
	logging.Warn(l.ctx, format, args...)
}

func (l *logger) Errorf(format string, args ...interface{}) {
	logging.Error(l.ctx, format, args...)
}

func (l *logger) Fatalf(format string, args ...interface{}) {
	logging.Fatal(l.ctx, format, args...)
}

func (l *logger) WithName(name string) log.Logger {
	return &logger{
		ctx: logging.SetLoggerScope(l.ctx, name),
	}
}
