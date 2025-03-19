package jaeger

import (
	"fmt"

	"github.com/uber/jaeger-client-go"

	"a.yandex-team.ru/library/go/core/log"
)

type logger struct {
	l log.Logger
}

var _ jaeger.Logger = &logger{}

func newLogger(l log.Logger) *logger {
	l = log.With(l, log.String("tracing_provider", "jaeger"))
	return &logger{
		l: l,
	}
}

// Error logs message at warning priority (tracing is not that important)
func (l *logger) Error(msg string) {
	l.l.Warnf("tracing: %s", msg)
}

// Infof logs message at debug priority
func (l *logger) Infof(msg string, args ...interface{}) {
	// They really are debug messages...
	l.l.Debugf(fmt.Sprintf("tracing: %s", msg), args...)
}
