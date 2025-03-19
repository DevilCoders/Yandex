package logger

import (
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/mattn/go-isatty"
	zp "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/yt/go/mapreduce"
)

// Дефолтный логгер.
// В ыте будет консольный.
// В nanny будет json-ый.
// В дев тачке будет прекрасный.
var Log log.Logger
var NullLog *zap.Logger

type Factory func(context.Context) log.Logger

func DummyLoggerFactory(ctx context.Context) log.Logger {
	return Log
}

func LoggerWithLevel(lvl zapcore.Level) log.Logger {
	cfg := zp.Config{
		Level:            zp.NewAtomicLevelAt(lvl),
		Encoding:         "console",
		OutputPaths:      []string{"stdout"},
		ErrorOutputPaths: []string{"stderr"},
		EncoderConfig: zapcore.EncoderConfig{
			MessageKey:     "msg",
			LevelKey:       "level",
			TimeKey:        "ts",
			CallerKey:      "caller",
			EncodeLevel:    zapcore.CapitalColorLevelEncoder,
			EncodeTime:     zapcore.ISO8601TimeEncoder,
			EncodeDuration: zapcore.StringDurationEncoder,
			EncodeCaller:   AdditionalComponentCallerEncoder,
		},
	}
	return log.With(zap.Must(cfg)).(*zap.Logger)
}

func AdditionalComponentCallerEncoder(caller zapcore.EntryCaller, enc zapcore.PrimitiveArrayEncoder) {
	path := caller.String()
	lastIndex := len(path) - 1
	for i := 0; i < 3; i++ {
		lastIndex = strings.LastIndex(path[0:lastIndex], "/")
		if lastIndex == -1 {
			break
		}
	}
	if lastIndex > 0 {
		path = path[lastIndex+1:]
	}
	enc.AppendString(path)
}

type levels struct {
	Zap zapcore.Level
	Log log.Level
}

func getEnvLogLevels() levels {
	zpLvl := zapcore.InfoLevel
	lvl := log.InfoLevel
	if level, ok := os.LookupEnv("LOG_LEVEL"); ok {
		return parseLevel(level)
	}
	return levels{zpLvl, lvl}
}

func parseLevel(level string) levels {
	zpLvl := zapcore.InfoLevel
	lvl := log.InfoLevel
	if level != "" {
		fmt.Printf("overriden log level to: %v\n", level)
		var l zapcore.Level
		if err := l.UnmarshalText([]byte(level)); err == nil {
			zpLvl = l
		}
		var gl log.Level
		if err := gl.UnmarshalText([]byte(level)); err == nil {
			lvl = gl
		}
	}
	return levels{zpLvl, lvl}
}

func copySlice(src []byte) []byte {
	dst := make([]byte, len(src))
	copy(dst, src)
	return dst
}

func copyBytes(source []byte) []byte {
	dup := make([]byte, len(source))
	copy(dup, source)
	return dup
}

func init() {
	level := getEnvLogLevels()

	encoder := zapcore.CapitalColorLevelEncoder
	if !isatty.IsTerminal(os.Stdout.Fd()) || !isatty.IsTerminal(os.Stderr.Fd()) {
		encoder = zapcore.CapitalLevelEncoder
	}

	cfg := zp.Config{
		Level:            zp.NewAtomicLevelAt(level.Zap),
		Encoding:         "console",
		OutputPaths:      []string{"stdout"},
		ErrorOutputPaths: []string{"stderr"},
		EncoderConfig: zapcore.EncoderConfig{
			MessageKey:     "msg",
			LevelKey:       "level",
			TimeKey:        "ts",
			CallerKey:      "caller",
			EncodeLevel:    encoder,
			EncodeTime:     zapcore.ISO8601TimeEncoder,
			EncodeDuration: zapcore.StringDurationEncoder,
			EncodeCaller:   AdditionalComponentCallerEncoder,
		},
	}

	if os.Getenv("QLOUD_LOGGER_STDOUT_PARSER") == "json" {
		cfg = zap.JSONConfig(level.Log)
	}

	if os.Getenv("CI") == "1" || strings.Contains(os.Args[0], "gotest") {
		cfg = zp.Config{
			Level:            zp.NewAtomicLevelAt(zp.DebugLevel),
			Encoding:         "console",
			OutputPaths:      []string{"stdout"},
			ErrorOutputPaths: []string{"stderr"},
			EncoderConfig: zapcore.EncoderConfig{
				MessageKey:     "msg",
				LevelKey:       "level",
				TimeKey:        "ts",
				CallerKey:      "caller",
				EncodeLevel:    zapcore.CapitalLevelEncoder,
				EncodeTime:     zapcore.ISO8601TimeEncoder,
				EncodeDuration: zapcore.StringDurationEncoder,
				EncodeCaller:   AdditionalComponentCallerEncoder,
			},
		}
	}
	if mapreduce.InsideJob() {
		cfg = zp.Config{
			Level:            cfg.Level,
			Encoding:         "console",
			OutputPaths:      []string{"stderr"},
			ErrorOutputPaths: []string{"stderr"},
			EncoderConfig: zapcore.EncoderConfig{
				MessageKey:     "msg",
				LevelKey:       "level",
				TimeKey:        "ts",
				CallerKey:      "caller",
				EncodeLevel:    zapcore.CapitalLevelEncoder,
				EncodeTime:     zapcore.ISO8601TimeEncoder,
				EncodeDuration: zapcore.StringDurationEncoder,
				EncodeCaller:   AdditionalComponentCallerEncoder,
			},
		}
	}

	host, _ := os.Hostname()
	Log = log.With(zap.Must(cfg), log.Any("host", host)).(*zap.Logger)
}
