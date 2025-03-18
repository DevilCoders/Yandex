package pgx

import (
	"context"

	"github.com/jackc/pgx/v4"

	"a.yandex-team.ru/library/go/core/log"
)

var _ pgx.Logger = new(logger)

type logger struct {
	l log.Structured
}

// NewLogger returns new pgx logger wrapper for log.Structured
//
// Example:
//     l := zap.Must(zap.JSONConfig(log.DebugLevel))
//
//     connConfig, _ := pgx.ParseConfig(postgresDSN)
//	   connConfig.Logger = pgxutil.NewLogger(l)
//	   connStr := stdlib.RegisterConnConfig(connConfig)
//
//	   conn, err := sql.Open("pgx", connStr)
func NewLogger(l log.Structured) *logger {
	return &logger{l: l}
}

// Log implements pgx.Logger interface
func (l *logger) Log(_ context.Context, level pgx.LogLevel, msg string, data map[string]interface{}) {
	field := log.Any("data", data)

	switch level {
	case pgx.LogLevelTrace, pgx.LogLevelDebug:
		l.l.Debug(msg, field)
	case pgx.LogLevelInfo:
		l.l.Info(msg, field)
	case pgx.LogLevelWarn:
		l.l.Warn(msg, field)
	case pgx.LogLevelError:
		l.l.Error(msg, field)
	default:
		l.l.Error(msg, field, log.String("PGX_LOG_LEVEL", level.String()))
	}
}
