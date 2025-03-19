package ctxlog

import (
	"bytes"
	"context"
	"encoding/json"
	"testing"
	"time"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

func TestGetLoggers(t *testing.T) {
	ctx := context.Background()
	if zap.L() != G(ctx) {
		t.Fatal("For an empty context Global logger must be returned")
	}
	if zap.S() != S(ctx) {
		t.Fatal("For an empty context Global Sugared logger must be returned")
	}
}

func TestTraceBitCore(t *testing.T) {
	const messageKey = "message"
	enc := zapcore.NewJSONEncoder(zapcore.EncoderConfig{
		MessageKey:     messageKey,
		EncodeDuration: zapcore.StringDurationEncoder,
	})
	lvl := zap.NewAtomicLevel()
	lvl.SetLevel(zapcore.ErrorLevel)
	buff := new(bytes.Buffer)
	extraFields := []zapcore.Field{zap.String("extra", "extra"), zap.Duration("d", time.Second)}

	ErrorLogger := zap.New(zapcore.NewCore(enc, zapcore.AddSync(buff), lvl))
	TraceBitLogger := withTraceBitCore(ErrorLogger)
	TraceBitLoggerWithField := TraceBitLogger.With(extraFields...)
	tbCtx := WithTraceBitLogger(WithLogger(context.Background(), ErrorLogger))

	fixtures := []struct {
		F           func(msg string, fields ...zapcore.Field)
		ShouldWrite bool
		Fields      []zapcore.Field
	}{
		// ErrorLogger should log messages with ErrorLevel only
		{ErrorLogger.Error, true, nil},
		{ErrorLogger.Warn, false, nil},
		{ErrorLogger.Info, false, nil},
		{ErrorLogger.Debug, false, nil},
		// TraceBitLogger should log all messages
		{TraceBitLogger.Error, true, nil},
		{TraceBitLogger.Warn, true, nil},
		{TraceBitLogger.Info, true, nil},
		{TraceBitLogger.Debug, true, nil},
		// TraceBitLoggerWithField should log all messages
		{TraceBitLoggerWithField.Error, true, extraFields},
		{TraceBitLoggerWithField.Warn, true, extraFields},
		{TraceBitLoggerWithField.Info, true, extraFields},
		{TraceBitLoggerWithField.Debug, true, extraFields},
		// TraceBitLogger from context should log all messages
		{G(tbCtx).Error, true, nil},
		{G(tbCtx).Warn, true, nil},
		{G(tbCtx).Info, true, nil},
		{G(tbCtx).Debug, true, nil},
	}

	for i, fixt := range fixtures {
		buff.Reset()
		fixtField := zap.Int("fixt", i)
		const textMessage = "text message"
		fixt.F(textMessage, fixtField)
		if !fixt.ShouldWrite {
			if buff.Len() != 0 {
				t.Fatalf("fixt %d: provided function is not expected to log data. %s", i, buff.String())
			}
			continue
		}

		var mp map[string]interface{}
		if err := json.NewDecoder(buff).Decode(&mp); err != nil {
			t.Fatalf("failed to decode fiedls %v", err)
		}

		if mp[messageKey] != textMessage {
			t.Fatal("malformed text message")
		}

		for _, field := range append(fixt.Fields, fixtField) {
			_, ok := mp[field.Key]
			if !ok {
				t.Fatalf("field %s is not logger", field.Key)
			}
		}
	}
}
