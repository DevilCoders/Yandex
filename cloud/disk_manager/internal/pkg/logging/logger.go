package logging

import (
	"context"
	"fmt"

	logging_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging/config"
	"a.yandex-team.ru/library/go/core/log"
)

////////////////////////////////////////////////////////////////////////////////

type Logger interface {
	log.Logger
}

////////////////////////////////////////////////////////////////////////////////

type Level = log.Level

const (
	TraceLevel = log.TraceLevel
	DebugLevel = log.DebugLevel
	InfoLevel  = log.InfoLevel
	WarnLevel  = log.WarnLevel
	ErrorLevel = log.ErrorLevel
	FatalLevel = log.FatalLevel
)

////////////////////////////////////////////////////////////////////////////////

type loggerKey struct{}
type loggerNameKey struct{}

func SetLogger(ctx context.Context, logger Logger) context.Context {
	return SetLoggingFields(context.WithValue(ctx, loggerKey{}, logger))
}

func SetLoggerScope(ctx context.Context, name string) context.Context {
	return SetLoggingFields(context.WithValue(ctx, loggerNameKey{}, name))
}

func GetLogger(ctx context.Context) Logger {
	logger, _ := ctx.Value(loggerKey{}).(Logger)
	if name, ok := ctx.Value(loggerNameKey{}).(string); ok {
		return logger.WithName(name)
	}
	return logger
}

////////////////////////////////////////////////////////////////////////////////

func CreateLogger(config *logging_config.LoggingConfig) Logger {
	switch logging := config.GetLogging().(type) {
	case *logging_config.LoggingConfig_LoggingStderr:
		return CreateStderrLogger(Level(config.GetLevel()))
	case *logging_config.LoggingConfig_LoggingJournald:
		return CreateJournaldLogger(Level(config.GetLevel()))
	default:
		panic(fmt.Errorf("unknown logger %v", logging))
	}
}
