package ctxlog

import (
	"context"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

// nolint:unparam
func condLog(l *zap.Logger, skipCallers int, levelOk zapcore.Level, levelErr zapcore.Level, err error, mess string, fields ...zap.Field) {
	skipCallers++
	logger := l.WithOptions(zap.AddCallerSkip(skipCallers))
	var level = levelOk
	if err != nil {
		level = levelErr

		// don't use fields = append(fields, zap.Error(err)) because if can broke outer fields in case
		// condLog(..., fields[:len(field)-n])
		newFields := make([]zap.Field, len(fields)+1)
		copy(newFields, fields)
		newFields[len(newFields)-1] = zap.Error(err)
		fields = newFields
	}
	if entry := logger.Check(level, mess); entry != nil {
		entry.Write(fields...)
	}
}

func DebugError(l *zap.Logger, err error, mess string, fields ...zap.Field) {
	condLog(l, 1, zapcore.DebugLevel, zapcore.ErrorLevel, err, mess, fields...)
}

func DebugErrorCtx(ctx context.Context, err error, mess string, fields ...zap.Field) {
	condLog(G(ctx), 1, zapcore.DebugLevel, zapcore.ErrorLevel, err, mess, fields...)
}

func Level(l *zap.Logger, level zapcore.Level, mess string, fields ...zap.Field) {
	condLog(l, 1, level, level, nil, mess, fields...)
}

func LevelCtx(ctx context.Context, level zapcore.Level, mess string, fields ...zap.Field) {
	l := G(ctx)
	condLog(l, 1, level, level, nil, mess, fields...)
}

func InfoDPanicCtx(ctx context.Context, err error, mess string, fields ...zap.Field) {
	l := G(ctx)
	condLog(l, 1, zapcore.InfoLevel, zapcore.DPanicLevel, err, mess, fields...)
}

func InfoError(l *zap.Logger, err error, mess string, fields ...zap.Field) {
	condLog(l, 1, zapcore.InfoLevel, zapcore.ErrorLevel, err, mess, fields...)
}

func InfoErrorCtx(ctx context.Context, err error, mess string, fields ...zap.Field) {
	l := G(ctx)
	condLog(l, 1, zapcore.InfoLevel, zapcore.ErrorLevel, err, mess, fields...)
}

func ErrorOnly(l *zap.Logger, err error, mess string, fields ...zap.Field) {
	if err == nil {
		return
	}

	condLog(l, 1, zapcore.ErrorLevel, zapcore.ErrorLevel, err, mess, fields...)
}

func DebugFatal(l *zap.Logger, err error, mess string, fields ...zap.Field) {
	condLog(l, 1, zapcore.DebugLevel, zapcore.FatalLevel, err, mess, fields...)
}
func DebugFatalCtx(ctx context.Context, err error, mess string, fields ...zap.Field) {
	l := G(ctx)
	condLog(l, 1, zapcore.DebugLevel, zapcore.FatalLevel, err, mess, fields...)
}
