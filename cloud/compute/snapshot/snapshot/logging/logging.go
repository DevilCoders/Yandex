package logging

import (
	"net/http"
	"os"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/crypto/ssh/terminal"
	"google.golang.org/grpc/grpclog"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/pkg/logging"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

// Journald means a journald socket as output.
const Journald = "journald"

var (
	logLevel = zap.NewAtomicLevel()
)

func init() {
	// register handler for log level switching
	http.DefaultServeMux.Handle("/switchloglevel", logLevel)
}

// We need this as workaround for
// https://github.com/uber-go/zap/issues/328
type syncWrapper struct {
	*os.File
}

func (s syncWrapper) Sync() error {
	_ = s.File.Sync()
	return nil
}

func open(path string) (ws zapcore.WriteSyncer, closeFunc func(), isTerminal bool, err error) {
	switch path {
	case "stdout", "/dev/stdout":
		ws := syncWrapper{os.Stdout}
		return zapcore.Lock(ws), func() {}, terminal.IsTerminal(int(os.Stdout.Fd())), nil
	case "stderr", "/dev/stderr":
		ws := syncWrapper{os.Stderr}
		return zapcore.Lock(ws), func() {}, terminal.IsTerminal(int(os.Stderr.Fd())), nil
	default:
		out, err1 := os.Open(path)
		if err1 != nil {
			return nil, nil, false, err1
		}
		isTerminal = terminal.IsTerminal(int(out.Fd()))
		ws, closeFunc, err = zap.Open(path)
		return
	}
}

// SetupLogging sets up zap logging according to config (journald-compatible).
func SetupLogging(cfg config.Config) (logger *zap.Logger, closeFunc func(), err error) {
	defer func() {
		if logger != nil {
			logger = logger.WithOptions(zap.Hooks(errorMessageHook))
		}
	}()
	if cfg.Logging.Output == Journald {
		logger, err = SetupJournaldLogging(cfg)
		return logger, func() {}, err
	}

	output, closeFunc, isTerminal, err := open(cfg.Logging.Output)
	if err != nil {
		zap.L().Error("failed to open output for logs", zap.String("output", cfg.Logging.Output), zap.Error(err))
		return nil, nil, err
	}

	// set logLevel
	// create encoder config
	// set configured logger as global
	// attach the logger to context
	logLevel.SetLevel(cfg.Logging.Level)
	encCfg := log.NewJournaldConfig().EncoderConfig
	if isTerminal {
		encCfg.TimeKey = "T"
		encCfg.EncodeLevel = logging.OneLetterColonLevelEncoder
		logger = zap.New(logging.NewColoredFileCore(
			logging.NewEncoder(encCfg, SnapshotMetaKeys),
			output, logLevel, logging.SnapshotColorMap))
	} else {
		logger = zap.New(logging.NewFileCore(
			logging.NewEncoder(encCfg, SnapshotMetaKeys),
			output, logLevel))
	}
	zap.ReplaceGlobals(logger)
	logger.Info("logger has been successfully configured")

	if cfg.Logging.EnableGrpc {
		grpcLogger, err := zap.NewStdLogAt(logger.With(zap.String("source", "grpclog")), zapcore.DebugLevel)
		if err != nil {
			zap.L().Error("failed to setup logging for grpclog")
			closeFunc()
			return nil, nil, err
		}
		//nolint:staticcheck
		//nolint:SA1019
		grpclog.SetLogger(grpcLogger)
	}

	return logger, closeFunc, nil
}

// SetupCliLogging sets up zap logging for CLI purpose (human-readable).
func SetupCliLogging() (logger *zap.Logger) {
	encCfg := log.NewJournaldConfig().EncoderConfig
	encCfg.TimeKey = "T"
	encCfg.EncodeLevel = logging.OneLetterColonLevelEncoder
	logger = zap.New(logging.NewFileCore(
		logging.NewEncoder(encCfg, SnapshotMetaKeys),
		os.Stdout, zapcore.DebugLevel))
	zap.ReplaceGlobals(logger)
	logger.Info("logger has been successfully configured")
	return logger
}

// SetupJournaldLogging sets up zap logging to journald socket.
func SetupJournaldLogging(cfg config.Config) (logger *zap.Logger, err error) {
	// set logLevel
	// create encoder config
	// set configured logger as global
	logLevel.SetLevel(cfg.Logging.Level)
	encCfg := log.NewJournaldConfig().EncoderConfig
	encCfg.EncodeLevel = logging.OneLetterColonLevelEncoder
	logger = zap.New(logging.NewJournaldCore(logging.NewEncoder(encCfg, SnapshotMetaKeys), logLevel))
	zap.ReplaceGlobals(logger)
	logger.Info("logger has been successfully configured")

	if cfg.Logging.EnableGrpc {
		grpcLogger, err := zap.NewStdLogAt(logger.With(zap.String("source", "grpclog")), zapcore.DebugLevel)
		if err != nil {
			zap.L().Error("failed to setup logging for grpclog")
			return nil, err
		}
		//nolint:staticcheck
		//nolint:SA1019
		grpclog.SetLogger(grpcLogger)
	}

	return logger, nil
}

func errorMessageHook(entry zapcore.Entry) error {
	if entry.Level >= zap.ErrorLevel {
		misc.LogErrorMessages.Inc()
	}
	return nil
}
