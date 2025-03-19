package logging

import (
	"fmt"
	"os"

	"github.com/coreos/go-systemd/journal"

	"a.yandex-team.ru/library/go/core/log"
)

////////////////////////////////////////////////////////////////////////////////

func getPriority(level Level) journal.Priority {
	switch level {
	case TraceLevel:
		return journal.PriDebug
	case DebugLevel:
		return journal.PriDebug
	case InfoLevel:
		return journal.PriInfo
	case WarnLevel:
		return journal.PriWarning
	case ErrorLevel:
		return journal.PriErr
	case FatalLevel:
		return journal.PriCrit
	}
	panic(fmt.Errorf("unknown log level %v", level))
}

////////////////////////////////////////////////////////////////////////////////

func getFields(fields ...log.Field) map[string]string {
	if len(fields) == 0 {
		return nil
	}

	vars := make(map[string]string)
	for _, f := range fields {
		vars[f.Key()] = fmt.Sprintf("%v", f.Any())
	}
	return vars
}

////////////////////////////////////////////////////////////////////////////////

type journaldLogger struct {
	level Level
	name  string
}

func (l *journaldLogger) structuredLog(
	level Level,
	msg string,
	fields ...log.Field,
) {
	if level < l.level {
		return
	}
	if len(l.name) > 0 {
		fields = append(fields, log.String("scope", l.name))
	}
	err := journal.Send(msg, getPriority(level), getFields(fields...))
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to write to journald %v\n", err)
	}
}

func (l *journaldLogger) Trace(msg string, fields ...log.Field) {
	l.structuredLog(TraceLevel, msg, fields...)
}

func (l *journaldLogger) Debug(msg string, fields ...log.Field) {
	l.structuredLog(DebugLevel, msg, fields...)
}

func (l *journaldLogger) Info(msg string, fields ...log.Field) {
	l.structuredLog(InfoLevel, msg, fields...)
}

func (l *journaldLogger) Warn(msg string, fields ...log.Field) {
	l.structuredLog(WarnLevel, msg, fields...)
}

func (l *journaldLogger) Error(msg string, fields ...log.Field) {
	l.structuredLog(ErrorLevel, msg, fields...)
}

func (l *journaldLogger) Fatal(msg string, fields ...log.Field) {
	l.structuredLog(FatalLevel, msg, fields...)
}

func (l *journaldLogger) fmtLog(
	level Level,
	format string,
	args ...interface{},
) {
	if level < l.level {
		return
	}
	l.structuredLog(level, fmt.Sprintf(format, args...))
}

func (l *journaldLogger) Tracef(format string, args ...interface{}) {
	l.fmtLog(TraceLevel, format, args...)
}

func (l *journaldLogger) Debugf(format string, args ...interface{}) {
	l.fmtLog(DebugLevel, format, args...)
}

func (l *journaldLogger) Infof(format string, args ...interface{}) {
	l.fmtLog(InfoLevel, format, args...)
}

func (l *journaldLogger) Warnf(format string, args ...interface{}) {
	l.fmtLog(WarnLevel, format, args...)
}

func (l *journaldLogger) Errorf(format string, args ...interface{}) {
	l.fmtLog(ErrorLevel, format, args...)
}

func (l *journaldLogger) Fatalf(format string, args ...interface{}) {
	l.fmtLog(FatalLevel, format, args...)
}

func (l *journaldLogger) Logger() log.Logger {
	return l
}

func (l *journaldLogger) Structured() log.Structured {
	return l
}

func (l *journaldLogger) Fmt() log.Fmt {
	return l
}

func (l *journaldLogger) WithName(name string) log.Logger {
	if len(l.name) > 0 {
		name = l.name + "." + name
	}
	return &journaldLogger{
		level: l.level,
		name:  name,
	}
}

////////////////////////////////////////////////////////////////////////////////

func CreateJournaldLogger(level Level) Logger {
	if !journal.Enabled() {
		panic(fmt.Errorf("cannot initialize journald logger"))
	}
	return &journaldLogger{
		level: level,
	}
}
