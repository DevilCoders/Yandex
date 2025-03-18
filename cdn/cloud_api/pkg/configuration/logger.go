package configuration

import (
	"fmt"
	"path/filepath"
	"sync"
	"syscall"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/logrotate"
)

var registerLogrotateSink sync.Once

func NewLogger(cfg LoggerConfig) (log.Logger, error) {
	var err error
	registerLogrotateSink.Do(func() {
		err = logrotate.RegisterLogrotateSink(syscall.SIGHUP)
	})
	if err != nil {
		return nil, fmt.Errorf("register logrotate sink: %w", err)
	}

	zapConfig := zap.StandardConfig(cfg.Encoding, cfg.Level)

	if cfg.Encoding == "deploy" {
		zapConfig.Encoding = "json"
		zapConfig.EncoderConfig = zap.NewDeployEncoderConfig()
	}

	var outputPath string
	switch cfg.Path {
	case "stdout", "stderr":
		outputPath = cfg.Path
	default:
		logPath, err := filepath.Abs(cfg.Path)
		if err != nil {
			return nil, fmt.Errorf("resolve path to log: %w", err)
		}

		outputPath = "logrotate://" + logPath
	}

	zapConfig.OutputPaths = []string{outputPath}

	logger, err := zap.New(zapConfig)
	if err != nil {
		return nil, fmt.Errorf("failed to construct logger: %w", err)
	}

	return logger, nil
}
