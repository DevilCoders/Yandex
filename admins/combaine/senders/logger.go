package senders

import (
	"fmt"
	"path/filepath"
	"syscall"

	"google.golang.org/grpc/grpclog"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/logrotate"
)

// Logger global for sender
var Logger = zap.Must(zap.ConsoleConfig(log.DebugLevel))

// InitializeLogger are non thread safe,
// call it only on init stage to initialize a global logger
func InitializeLogger(level string, path string) {
	err := logrotate.RegisterLogrotateSink(syscall.SIGHUP)
	if err != nil {
		panic(err)
	}
	l, err := log.ParseLevel(level)
	if err != nil {
		panic(err)
	}
	cfg := zap.KVConfig(l)
	logPath, err := filepath.Abs(path)
	if err != nil {
		panic(err)
	}

	cfg.OutputPaths = []string{"logrotate://" + logPath}
	Logger = zap.Must(cfg)
}

type grpcLogger struct {
	l *zap.Logger
	v int
}

// NewGRPCLoggerV2WithVerbosity wrap grpc -> zap logger
func NewGRPCLoggerV2WithVerbosity(v int) grpclog.LoggerV2 {
	return &grpcLogger{l: Logger, v: v}
}

func (g *grpcLogger) Info(args ...interface{}) {
	g.l.Info(fmt.Sprint(args...))
}

func (g *grpcLogger) Infoln(args ...interface{}) {
	g.l.Info(fmt.Sprintln(args...))
}

func (g *grpcLogger) Infof(format string, args ...interface{}) {
	g.l.Infof(format, args...)
}

func (g *grpcLogger) Warning(args ...interface{}) {
	g.l.Warn(fmt.Sprint(args...))
}

func (g *grpcLogger) Warningln(args ...interface{}) {
	g.l.Warn(fmt.Sprintln(args...))
}

func (g *grpcLogger) Warningf(format string, args ...interface{}) {
	g.l.Warnf(format, args...)
}

func (g *grpcLogger) Error(args ...interface{}) {
	g.l.Error(fmt.Sprint(args...))
}

func (g *grpcLogger) Errorln(args ...interface{}) {
	g.l.Error(fmt.Sprintln(args...))
}

func (g *grpcLogger) Errorf(format string, args ...interface{}) {
	g.l.Errorf(format, args...)
}

func (g *grpcLogger) Fatal(args ...interface{}) {
	g.l.Fatal(fmt.Sprint(args...))
	// No need to call os.Exit() again because log.Logger.Fatal() calls os.Exit().
}

func (g *grpcLogger) Fatalln(args ...interface{}) {
	g.l.Fatal(fmt.Sprintln(args...))
	// No need to call os.Exit() again because log.Logger.Fatal() calls os.Exit().
}

func (g *grpcLogger) Fatalf(format string, args ...interface{}) {
	g.l.Fatalf(format, args...)
	// No need to call os.Exit() again because log.Logger.Fatal() calls os.Exit().
}

func (g *grpcLogger) V(l int) bool {
	return l <= g.v
}
