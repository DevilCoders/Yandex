package logger

import (
	"context"

	"go.uber.org/zap"
)

type key int

var loggerKey key

func FromContext(ctx context.Context) *zap.Logger {
	if ctx == nil {
		return zap.NewNop()
	}

	v := ctx.Value(loggerKey)
	if v == nil {
		return zap.NewNop()
	}

	return v.(*zap.Logger)
}

func NewContext(ctx context.Context, l *zap.Logger) context.Context {
	return context.WithValue(ctx, loggerKey, l)
}
