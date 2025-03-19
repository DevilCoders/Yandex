package kmslbclient

import (
	"bytes"
	"fmt"
	"log"
	"os"

	alog "a.yandex-team.ru/library/go/core/log"
)

type loggerFromLog struct {
	log   *log.Logger
	level alog.Level
}

func NewLoggerFromLog(log *log.Logger, level alog.Level) alog.Logger {
	return &loggerFromLog{log: log, level: level}
}

func (l *loggerFromLog) structuredLog(
	level alog.Level,
	msg string,
	fields ...alog.Field,
) {
	if level < l.level {
		return
	}
	data := new(bytes.Buffer)
	fmt.Fprintf(data, "%s=%s", "logLevel", level.String())
	fmt.Fprintf(data, " %s=%s", "message", msg)
	for k, v := range getFields(fields...) {
		fmt.Fprintf(data, " %s=%s", k, v)
	}
	l.log.Println(data.String())
}

func (l *loggerFromLog) Trace(msg string, fields ...alog.Field) {
	l.structuredLog(alog.TraceLevel, msg, fields...)
}

func (l *loggerFromLog) Debug(msg string, fields ...alog.Field) {
	l.structuredLog(alog.DebugLevel, msg, fields...)
}

func (l *loggerFromLog) Info(msg string, fields ...alog.Field) {
	l.structuredLog(alog.InfoLevel, msg, fields...)
}

func (l *loggerFromLog) Warn(msg string, fields ...alog.Field) {
	l.structuredLog(alog.WarnLevel, msg, fields...)
}

func (l *loggerFromLog) Error(msg string, fields ...alog.Field) {
	l.structuredLog(alog.ErrorLevel, msg, fields...)
}

func (l *loggerFromLog) Fatal(msg string, fields ...alog.Field) {
	l.structuredLog(alog.FatalLevel, msg, fields...)
	os.Exit(1)
}

func (l *loggerFromLog) fmtLog(
	level alog.Level,
	format string,
	args ...interface{},
) {
	if level < l.level {
		return
	}
	l.log.Printf(format, args...)
}

func (l *loggerFromLog) Tracef(format string, args ...interface{}) {
	l.fmtLog(alog.TraceLevel, "TRACE: "+format, args...)
}

func (l *loggerFromLog) Debugf(format string, args ...interface{}) {
	l.fmtLog(alog.DebugLevel, "DEBUG: "+format, args...)
}

func (l *loggerFromLog) Infof(format string, args ...interface{}) {
	l.fmtLog(alog.InfoLevel, "INFO: "+format, args...)
}

func (l *loggerFromLog) Warnf(format string, args ...interface{}) {
	l.fmtLog(alog.WarnLevel, "WARN: "+format, args...)
}

func (l *loggerFromLog) Errorf(format string, args ...interface{}) {
	l.fmtLog(alog.ErrorLevel, "ERROR: "+format, args...)
}

func (l *loggerFromLog) Fatalf(format string, args ...interface{}) {
	l.fmtLog(alog.FatalLevel, "FATAL: "+format, args...)
	os.Exit(1)
}

func (l *loggerFromLog) Structured() alog.Structured {
	return l
}

func (l *loggerFromLog) Logger() alog.Logger {
	return l
}

func (l *loggerFromLog) Fmt() alog.Fmt {
	return l
}

func (l *loggerFromLog) WithName(name string) alog.Logger {
	return l
}

func getFields(fields ...alog.Field) map[string]string {
	if len(fields) == 0 {
		return nil
	}

	vars := make(map[string]string)
	for _, f := range fields {
		vars[f.Key()] = fmt.Sprintf("%v", f.Any())
	}
	return vars
}
