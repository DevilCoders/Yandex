package logger

import (
	"context"
	"os"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func New() (*zap.Logger, *zap.AtomicLevel) {
	level := zap.NewAtomicLevel()
	cores := zapcore.NewCore(
		zapcore.NewConsoleEncoder(defaultEncoderConfig),
		zapcore.Lock(os.Stdout),
		level)

	return zap.New(cores), &level
}

var defaultEncoderConfig = zapcore.EncoderConfig{
	MessageKey:     "msg",
	LevelKey:       "level",
	TimeKey:        "ts",
	CallerKey:      "caller",
	NameKey:        "name",
	StacktraceKey:  "strace",
	EncodeLevel:    zapcore.CapitalLevelEncoder,
	EncodeTime:     zapcore.ISO8601TimeEncoder,
	EncodeDuration: zapcore.StringDurationEncoder,
	EncodeCaller:   zapcore.ShortCallerEncoder,
}

const callerSkipNum = 2

var debugOptions = []zap.Option{zap.AddCallerSkip(callerSkipNum), zap.AddCaller(), zap.AddStacktrace(zapcore.DebugLevel)}

func InfoCtx(ctx context.Context, msg string, fields ...zap.Field) {
	log(FromContext(ctx), msg, zapcore.InfoLevel, fields...)
}

func DebugCtx(ctx context.Context, msg string, fields ...zap.Field) {
	log(FromContext(ctx).WithOptions(debugOptions...), msg, zapcore.DebugLevel, fields...)
}

func FatalCtx(ctx context.Context, msg string, fields ...zap.Field) {
	FromContext(ctx).WithOptions(debugOptions...).Fatal(msg, fields...)
}

func log(l *zap.Logger, msg string, level zapcore.Level, fields ...zap.Field) {
	if ce := l.Check(level, msg); ce != nil {
		ce.Write(fields...)
	}
}
