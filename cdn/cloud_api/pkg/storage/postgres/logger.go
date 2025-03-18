package postgres

import (
	"context"
	"time"

	"gorm.io/gorm/logger"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Logger struct {
	logger log.Logger
}

func NewLogger(logger log.Logger) *Logger {
	return &Logger{
		logger: logger,
	}
}

func (l *Logger) LogMode(_ logger.LogLevel) logger.Interface {
	return l
}

func (l *Logger) Info(ctx context.Context, s string, i ...interface{}) {
	ctxlog.Infof(ctx, l.logger, s, i...)
}

func (l *Logger) Warn(ctx context.Context, s string, i ...interface{}) {
	ctxlog.Warnf(ctx, l.logger, s, i...)
}

func (l *Logger) Error(ctx context.Context, s string, i ...interface{}) {
	ctxlog.Errorf(ctx, l.logger, s, i...)
}

func (l *Logger) Trace(ctx context.Context, begin time.Time, fc func() (sql string, rowsAffected int64), err error) {
	sql, rows := fc()

	fields := []log.Field{
		log.Time("begin", begin),
		log.String("sql", sql),
		log.Int64("rows", rows),
	}

	if err != nil {
		fields = append(fields, log.Error(err))
	}

	ctxlog.Debug(ctx, l.logger, "trace", fields...)
}
