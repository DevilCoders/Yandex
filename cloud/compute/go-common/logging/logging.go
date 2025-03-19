package logging

import (
	"bytes"
	"fmt"
	"log"
	"os"

	"github.com/coreos/go-systemd/journal"
	"go.uber.org/zap"
	"golang.org/x/crypto/ssh/terminal"

	"a.yandex-team.ru/cloud/compute/go-common/logging/go-logging"
)

type Config struct {
	DebugMode   bool
	DevelopMode bool
}

type JournaldLogBackend struct {
}

func NewJournaldLogBackend() *JournaldLogBackend {
	return &JournaldLogBackend{}
}

var levelToJournaldPriorityMap = map[logging.Level]journal.Priority{
	logging.CRITICAL: journal.PriCrit,
	logging.ERROR:    journal.PriErr,
	logging.WARNING:  journal.PriWarning,
	logging.NOTICE:   journal.PriNotice,
	logging.INFO:     journal.PriInfo,
	logging.DEBUG:    journal.PriDebug,
}

func (b *JournaldLogBackend) Log(level logging.Level, calldepth int, rec *logging.Record) error {
	pri, ok := levelToJournaldPriorityMap[level]
	if !ok {
		pri = journal.PriAlert
	}
	return journal.Send(rec.Formatted(calldepth+1), pri, nil)
}

func MustInitLogging(config Config) {
	var zapConfig zap.Config

	level := INFO
	if config.DebugMode {
		level = DEBUG
	}

	format := ""
	var backend logging.Backend
	if config.DevelopMode {
		format += "[%{time:2006.01.02 15:04:05.000}] [%{shortfile}]"
		zapConfig = zap.NewDevelopmentConfig()
		file := os.Stderr
		fileBackend := logging.NewLogBackend(file, "", 0)
		if terminal.IsTerminal(int(file.Fd())) {
			fileBackend.ColorConfig = []string{
				logging.CRITICAL: logging.ColorSeq(logging.ColorRed),
				logging.ERROR:    logging.ColorSeq(logging.ColorRed),
				logging.WARNING:  logging.ColorSeq(logging.ColorYellow),
				logging.NOTICE:   logging.ColorSeq(logging.ColorYellow),
				logging.INFO:     logging.ColorSeq(logging.ColorGreen),
				logging.DEBUG:    logging.ColorSeq(logging.ColorCyan),
			}
			fileBackend.Color = true
		}
		backend = fileBackend
	} else {
		format += "[%{module}]"
		zapConfig = zap.NewProductionConfig()
		backend = NewJournaldLogBackend()
	}

	format += " %{level:.1s}: %{message}"

	logging.SetBackend(backend)
	logging.SetFormatter(logging.MustStringFormatter(format))
	logging.SetLevel(level.implementationLevel(), "")

	// TODO: For now always set this severity to suppress excessive logs from gokikimr
	zapConfig.Level.SetLevel(zap.WarnLevel)
	zapConfig.DisableStacktrace = true

	if logger, err := zapConfig.Build(); err == nil {
		zap.ReplaceGlobals(logger)
	} else {
		panic(fmt.Sprintf("Unable to initialize the logging: %s.", err))
	}
}

type Level int

const (
	CRITICAL Level = iota
	ERROR
	WARNING
	INFO
	DEBUG
)

var implementationLevelMapping = []logging.Level{
	logging.CRITICAL,
	logging.ERROR,
	logging.WARNING,
	logging.INFO,
	logging.DEBUG,
}

func (level Level) implementationLevel() logging.Level {
	return implementationLevelMapping[int(level)]
}

type Logger struct {
	extraCallDepth int
	logger         *logging.Logger
}

func GetLogger(name string) *Logger {
	return getLogger(name, 0)
}

func getLogger(name string, extraCalldepth int) *Logger {
	logger := logging.MustGetLogger(name)
	logger.ExtraCalldepth = 2 + extraCalldepth

	return &Logger{
		extraCallDepth: extraCalldepth,
		logger:         logger,
	}
}

func (l *Logger) CloneWithIncreasedExtraCallDepth(extraCallDepth int) *Logger {
	return getLogger(l.logger.Module, l.extraCallDepth+extraCallDepth)
}

func (l *Logger) IsEnabledFor(level Level) bool {
	return l.logger.IsEnabledFor(level.implementationLevel())
}

func (l *Logger) Debug(format string, args ...interface{}) {
	l.log(DEBUG, format, args...)
}

func (l *Logger) Info(format string, args ...interface{}) {
	l.log(INFO, format, args...)
}

func (l *Logger) Warning(format string, args ...interface{}) {
	l.log(WARNING, format, args...)
}

func (l *Logger) Error(format string, args ...interface{}) {
	l.log(ERROR, format, args...)
}

func (l *Logger) Critical(format string, args ...interface{}) {
	l.log(CRITICAL, format, args...)
}

func (l *Logger) Panic(format string, args ...interface{}) {
	l.log(CRITICAL, format, args...)
	panic(fmt.Sprintf(format, args...))
}

func (l *Logger) Fatal(format string, args ...interface{}) {
	l.log(CRITICAL, format, args...)
	os.Exit(1)
}

func (l *Logger) Log(level Level, format string, args ...interface{}) {
	// This wrapper is needed to preserve the same call depth
	l.log(level, format, args...)
}

func (l *Logger) log(level Level, format string, args ...interface{}) {
	switch level {
	case DEBUG:
		l.logger.Debugf(format, args...)
	case INFO:
		l.logger.Infof(format, args...)
	case WARNING:
		l.logger.Warningf(format, args...)
	case ERROR:
		l.logger.Errorf(format, args...)
	case CRITICAL:
		l.logger.Criticalf(format, args...)
	default:
		panic("Logical error")
	}
}

type httpServerErrorLogWriter struct {
	logger *Logger
}

func (l *httpServerErrorLogWriter) Write(line []byte) (int, error) {
	l.logger.Error("%s", bytes.TrimRight(line, "\n"))
	return len(line), nil
}

func GetHTTPServerErrorLogger(name string) *log.Logger {
	return log.New(&httpServerErrorLogWriter{getLogger(name, 4)}, "", 0)
}

// Logging adapter for github.com/jackc/pgx
type PgxLogger struct {
	logger *logging.Logger
}

func GetPgxLogger() *PgxLogger {
	logger := PgxLogger{logger: logging.MustGetLogger("pgx")}
	logger.logger.ExtraCalldepth = 2
	return &logger
}

func (l *PgxLogger) Debug(msg string, ctx ...interface{}) {
	l.log(logging.DEBUG, msg, ctx)
}

func (l *PgxLogger) Info(msg string, ctx ...interface{}) {
	l.log(logging.INFO, msg, ctx)
}

func (l *PgxLogger) Warn(msg string, ctx ...interface{}) {
	l.log(logging.WARNING, msg, ctx)
}

func (l *PgxLogger) Error(msg string, ctx ...interface{}) {
	l.log(logging.ERROR, msg, ctx)
}

func (l *PgxLogger) log(level logging.Level, msg string, ctx ...interface{}) {
	if !l.logger.IsEnabledFor(level) {
		return
	}

	args := make([]interface{}, 0, 1+len(ctx))
	args = append(args, msg)
	args = append(args, ctx...)

	switch level {
	case logging.DEBUG:
		l.logger.Debug(args...)
	case logging.INFO:
		l.logger.Info(args...)
	case logging.WARNING:
		l.logger.Warning(args...)
	default:
		l.logger.Error(args...)
	}
}
