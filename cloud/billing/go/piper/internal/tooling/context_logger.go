package tooling

import (
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logging"
	"a.yandex-team.ru/library/go/core/log"
)

type ctxLogger struct {
	store  *ctxStore
	logger log.Structured
}

func (s *ctxStore) logger() ctxLogger {
	return ctxLogger{
		store:  s,
		logger: logging.Logger(),
	}
}

func (l ctxLogger) Trace(msg string, fields ...log.Field) {
	l.log(l.logger.Trace, msg, fields)
}

func (l ctxLogger) Debug(msg string, fields ...log.Field) {
	l.log(l.logger.Debug, msg, fields)
}

func (l ctxLogger) Info(msg string, fields ...log.Field) {
	l.log(l.logger.Info, msg, fields)
}

func (l ctxLogger) Warn(msg string, fields ...log.Field) {
	l.log(l.logger.Warn, msg, fields)
}

func (l ctxLogger) Error(msg string, fields ...log.Field) {
	l.log(l.logger.Error, msg, fields)
}

func (l ctxLogger) Fatal(msg string, fields ...log.Field) {
	l.log(l.logger.Fatal, msg, fields)
}

func (l ctxLogger) Fmt() log.Fmt {
	panic("only structured logging allowed")
}

func (l ctxLogger) Logger() log.Logger {
	return log.With(l.logger.Logger(), l.store.addLogFields(nil)...)
}

func (l ctxLogger) log(call func(string, ...log.Field), msg string, callFields []log.Field) {
	fields := logging.GetFields()
	defer logging.PutFields(fields)

	fields.Fields = l.store.addLogFields(fields.Fields)
	fields.Fields = append(fields.Fields, callFields...)

	call(msg, fields.Fields...)
}

func (s *ctxStore) addLogFields(f []log.Field) []log.Field {
	f = append(f,
		logf.Service(s.service),
	)
	if s.sourceFull != "" {
		f = append(f, logf.SourceName(s.sourceFull))
	}
	if s.requestID != "" {
		f = append(f, logf.RequestID(s.requestID))
	}
	if s.handler != "" {
		f = append(f, logf.Handler(s.handler))
	}
	if s.action != "" {
		f = append(f, logf.Action(s.action))
	}
	if s.dbQueryName != "" {
		f = append(f, logf.QueryName(s.dbQueryName))
	}
	if s.dbQueryRowsCount != 0 {
		f = append(f, logf.QueryRows(s.dbQueryRowsCount))
	}
	if s.icRequest != "" {
		f = append(f, logf.RequestName(s.icSystem, s.icRequest))
	}
	if s.retryStarted {
		f = append(f, logf.RetryAttempt(s.retryAttempt))
	}

	return f
}
