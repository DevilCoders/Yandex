package ctxlog

import (
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func journaldLevelEncoder(lvl zapcore.Level, enc zapcore.PrimitiveArrayEncoder) {
	const (
		SDEmerg = "<0>" /* system is unusable */
		// SDAlert = "<1>" /* action must be taken immediately */
		SDCrit    = "<2>" /* critical conditions */
		SDErr     = "<3>" /* error conditions */
		SDWarning = "<4>" /* warning conditions */
		// SDNotice = "<5>" /* normal but significant condition */
		SDInfo  = "<6>" /* informational */
		SDDebug = "<7>" /* debug-level messages */
	)
	switch lvl {
	case zap.DebugLevel:
		enc.AppendString(SDDebug)
	case zap.InfoLevel:
		enc.AppendString(SDInfo)
	case zap.WarnLevel:
		enc.AppendString(SDWarning)
	case zap.ErrorLevel:
		enc.AppendString(SDErr)
	case zap.DPanicLevel, zap.PanicLevel:
		enc.AppendString(SDCrit)
	case zap.FatalLevel:
		enc.AppendString(SDEmerg)
	}
}

// NewJournald builds logger from Jounrnald config
func NewJournald() (*zap.Logger, error) {
	return NewJournaldConfig().Build()
}

// NewJournaldConfig returns config, that constructs Logger tuned for Systemd + Jounrnald stack
func NewJournaldConfig() zap.Config {
	dyn := zap.NewAtomicLevel()
	dyn.SetLevel(zap.DebugLevel)
	return zap.Config{
		Level:       dyn,
		Development: false,
		Encoding:    "console",
		EncoderConfig: zapcore.EncoderConfig{
			// Keys can be anything except the empty string.
			TimeKey:        "", // we do not need time
			LevelKey:       "L",
			NameKey:        "N",
			CallerKey:      "",
			MessageKey:     "M",
			StacktraceKey:  "S",
			EncodeLevel:    journaldLevelEncoder,
			EncodeTime:     zapcore.ISO8601TimeEncoder,
			EncodeDuration: zapcore.StringDurationEncoder,
		},
		OutputPaths:      []string{"stdout"},
		ErrorOutputPaths: []string{"stdout"},
	}
}
