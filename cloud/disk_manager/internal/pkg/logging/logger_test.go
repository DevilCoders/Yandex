package logging

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/library/go/core/log"
)

////////////////////////////////////////////////////////////////////////////////

type mockLogger struct {
	mock.Mock
}

func (l *mockLogger) Trace(msg string, fields ...log.Field) {
	l.Called(msg, fields)
}

func (l *mockLogger) Debug(msg string, fields ...log.Field) {
	l.Called(msg, fields)
}

func (l *mockLogger) Info(msg string, fields ...log.Field) {
	l.Called(msg, fields)
}

func (l *mockLogger) Warn(msg string, fields ...log.Field) {
	l.Called(msg, fields)
}

func (l *mockLogger) Error(msg string, fields ...log.Field) {
	l.Called(msg, fields)
}

func (l *mockLogger) Fatal(msg string, fields ...log.Field) {
	l.Called(msg, fields)
}

func (l *mockLogger) Tracef(format string, args ...interface{}) {
	l.Called(format, args)
}

func (l *mockLogger) Debugf(format string, args ...interface{}) {
	l.Called(format, args)
}

func (l *mockLogger) Infof(format string, args ...interface{}) {
	l.Called(format, args)
}

func (l *mockLogger) Warnf(format string, args ...interface{}) {
	l.Called(format, args)
}

func (l *mockLogger) Errorf(format string, args ...interface{}) {
	l.Called(format, args)
}

func (l *mockLogger) Fatalf(format string, args ...interface{}) {
	l.Called(format, args)
}

func (l *mockLogger) Logger() log.Logger {
	return l
}

func (l *mockLogger) Structured() log.Structured {
	return l
}

func (l *mockLogger) Fmt() log.Fmt {
	return l
}

func (l *mockLogger) WithName(name string) log.Logger {
	return l
}

func createMockLogger() *mockLogger {
	return &mockLogger{}
}

////////////////////////////////////////////////////////////////////////////////

func createContext(idempotencyKey string, requestID string) context.Context {
	return headers.SetIncomingRequestID(
		headers.SetIncomingIdempotencyKey(context.Background(), idempotencyKey),
		requestID,
	)
}

////////////////////////////////////////////////////////////////////////////////

func TestLogTrace(t *testing.T) {
	logger := createMockLogger()
	ctx := SetLogger(createContext("idempID", "reqID"), logger)

	logger.On(
		"Trace",
		"Stuff happened",
		[]log.Field{
			log.String("IDEMPOTENCY_KEY", "idempID"),
			log.String("REQUEST_ID", "reqID"),
		},
	)

	Trace(ctx, "Stuff happened")
}

func TestLogDebug(t *testing.T) {
	logger := createMockLogger()
	ctx := SetLogger(createContext("idempID", "reqID"), logger)

	logger.On(
		"Debug",
		"Stuff happened",
		[]log.Field{
			log.String("IDEMPOTENCY_KEY", "idempID"),
			log.String("REQUEST_ID", "reqID"),
		},
	)

	Debug(ctx, "Stuff happened")
}

func TestLogInfo(t *testing.T) {
	logger := createMockLogger()
	ctx := SetLogger(createContext("idempID", "reqID"), logger)

	logger.On(
		"Info",
		"Stuff happened",
		[]log.Field{
			log.String("IDEMPOTENCY_KEY", "idempID"),
			log.String("REQUEST_ID", "reqID"),
		},
	)

	Info(ctx, "Stuff happened")
}

func TestLogWarn(t *testing.T) {
	logger := createMockLogger()
	ctx := SetLogger(createContext("idempID", "reqID"), logger)

	logger.On(
		"Warn",
		"Stuff happened",
		[]log.Field{
			log.String("IDEMPOTENCY_KEY", "idempID"),
			log.String("REQUEST_ID", "reqID"),
		},
	)

	Warn(ctx, "Stuff happened")
}

func TestLogError(t *testing.T) {
	logger := createMockLogger()
	ctx := SetLogger(createContext("idempID", "reqID"), logger)

	logger.On(
		"Error",
		"Stuff happened",
		[]log.Field{
			log.String("IDEMPOTENCY_KEY", "idempID"),
			log.String("REQUEST_ID", "reqID"),
		},
	)

	Error(ctx, "Stuff happened")
}

func TestLogFatal(t *testing.T) {
	logger := createMockLogger()
	ctx := SetLogger(createContext("idempID", "reqID"), logger)

	logger.On(
		"Fatal",
		"Stuff happened",
		[]log.Field{
			log.String("IDEMPOTENCY_KEY", "idempID"),
			log.String("REQUEST_ID", "reqID"),
		},
	)

	Fatal(ctx, "Stuff happened")
}
