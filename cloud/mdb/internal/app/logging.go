package app

import (
	"path/filepath"
	"sync"
	"syscall"

	z "go.uber.org/zap"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/logrotate"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type LoggerConstructor = func(cfg LoggingConfig) (log.Logger, error)

var DefaultLoggerConstructor = DefaultToolLoggerConstructor

// DefaultServiceLoggerConstructor returns logger constructor suitable for most services
func DefaultServiceLoggerConstructor() LoggerConstructor {
	return func(cfg LoggingConfig) (log.Logger, error) {
		return configureZap(zap.JSONConfig(cfg.Level), cfg.File)
	}
}

// DefaultToolLoggerConstructor returns logger constructor suitable for most tools.
// Prints time at the start of the message.
func DefaultToolLoggerConstructor() LoggerConstructor {
	return func(cfg LoggingConfig) (log.Logger, error) {
		return configureZap(zap.ConsoleConfig(cfg.Level), cfg.File)
	}
}

// DefaultCLILoggerConstructor returns logger constructor suitable for most CLI-tools.
// It does not print message's time.
func DefaultCLILoggerConstructor() LoggerConstructor {
	return func(cfg LoggingConfig) (log.Logger, error) {
		config := zap.CLIConfig(cfg.Level)
		config.ErrorOutputPaths = config.OutputPaths // print errors to stdout
		return configureZap(config, cfg.File)
	}
}

var sinkOnce sync.Once

func configureZap(cfg z.Config, file string) (log.Logger, error) {
	if file == "" {
		return zap.New(cfg)
	}

	// Register logrotate sink
	// This can be called only once
	var err error
	sinkOnce.Do(
		func() {
			err = logrotate.RegisterLogrotateSink(syscall.SIGHUP)
		},
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to register logrotate sink: %w", err)
	}

	logPath, err := filepath.Abs(file)
	if err != nil {
		return nil, xerrors.Errorf("invalid log_file path: %w", err)
	}

	rotatePath := "logrotate://" + logPath
	cfg.OutputPaths = append(cfg.OutputPaths, rotatePath)
	cfg.ErrorOutputPaths = append(cfg.ErrorOutputPaths, rotatePath)
	return zap.New(cfg)
}
