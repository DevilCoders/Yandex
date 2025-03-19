package ctxlog

import (
	"context"

	"go.uber.org/zap"
)

func Example() {
	config := NewJournaldConfig()
	logger, err := config.Build()
	if err != nil {
		panic(err)
	}
	config.Level.SetLevel(zap.InfoLevel)

	ctx := WithLogger(context.Background(), logger)
	// Using fastest zap.Logger methods
	G(ctx).Info("message", zap.String("Key", "Value"), zap.Int("IntKey", 1))
	// Zap style, but without explicit field types
	S(ctx).Infow("message", "Key", "Value", "IntKey", 1)
	// SugaredLogger with fmt.Printf style
	S(ctx).Infof("message %s %d", "Value", 1)
	// won't print this
	S(ctx).Debug("debug is not enabled for this logger")
	// TraceBitLogger is used when the request must be handled with increased verbosity
	trCtx := WithTraceBitLogger(ctx)
	S(trCtx).Debug("traced")
	// Output:
	// <6>	message	{"Key": "Value", "IntKey": 1}
	// <6>	message	{"Key": "Value", "IntKey": 1}
	// <6>	message Value 1
	// <7>	traced
}
