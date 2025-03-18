package uazap

import (
	"time"

	"go.uber.org/zap/zapcore"
	"golang.org/x/time/rate"

	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/metrics"
)

const mb = 1 << 20

type Option func(o *options)

type options struct {
	flushInterval  time.Duration
	maxMemoryUsage int
	rateLimiter    *rate.Limiter
	metaExtractor  func(entry zapcore.Entry, fields []zapcore.Field) map[string]string
	metricsCollect func(core *Core)
}

func (o *options) setDefaults() {
	o.flushInterval = 10 * time.Millisecond
	o.maxMemoryUsage = 10 * mb
	o.rateLimiter = rate.NewLimiter(rate.Inf, 0)
	o.metaExtractor = PriorityMetaExtractor
	o.metricsCollect = func(*Core) {}
}

func PriorityMetaExtractor(entry zapcore.Entry, _ []zapcore.Field) map[string]string {
	return map[string]string{"_priority": zap.UnzapifyLevel(entry.Level).String()}
}

// WithFlushInterval changes maximum time between logging and actual client RPC of unified agent.
// Default: 10ms.
func WithFlushInterval(interval time.Duration) Option {
	return func(o *options) {
		if interval <= 0 {
			return
		}
		o.flushInterval = interval
	}
}

// WithMaxMemoryUsage sets memory limit for logging infrastructure.
// Default: 10MB.
func WithMaxMemoryUsage(numBytes int) Option {
	return func(o *options) {
		o.maxMemoryUsage = numBytes
	}
}

// WithMetaExtractor sets custom metadata extractor based on collected log record.
// Default: PriorityMetaExtractor.
func WithMetaExtractor(extractor func(entry zapcore.Entry, fields []zapcore.Field) map[string]string) Option {
	return func(o *options) {
		if extractor == nil {
			extractor = func(zapcore.Entry, []zapcore.Field) map[string]string { return nil }
		}
		o.metaExtractor = extractor
	}
}

// WithRateLimiter sets rate limiter for data per second. Records exceeding limit are dropped.
// rate.NewLimiter(r rate.Limit, b int) means r bytes per second
// with a maximum burst of b bytes in one message (including metadata).
// Default: no limit.
func WithRateLimiter(limiter *rate.Limiter) Option {
	return func(o *options) {
		o.rateLimiter = limiter
	}
}

// WithMetrics adds metrics collection from core and client.
func WithMetrics(r metrics.Registry, c metrics.CollectPolicy) Option {
	return func(o *options) {
		o.metricsCollect = CoreMetricsCollector(r, c)
	}
}
