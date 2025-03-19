package retry

import (
	"context"
	"time"
)

// Option for Retry
type Option func(Config) Config

// WithMaxRetries sets the allowed number of retries. Zero means infinity.
func WithMaxRetries(tries uint64) Option {
	return func(c Config) Config {
		c.MaxRetries = tries
		return c
	}
}

// WithInitialInterval sets initial timeout between tries.
func WithInitialInterval(t time.Duration) Option {
	return func(c Config) Config {
		c.InitialInterval = t
		return c
	}
}

// WithMaxInterval sets maximum timeout between tries.
func WithMaxInterval(t time.Duration) Option {
	return func(c Config) Config {
		c.MaxInterval = t
		return c
	}
}

// WithMaxElapsedTime sets timeout for total execution duration. It is NOT checked while operation is executing.
func WithMaxElapsedTime(t time.Duration) Option {
	return func(c Config) Config {
		c.MaxElapsedTime = t
		return c
	}
}

// Retry given operation using supplied options.
//
// This is a convenience method, not supposed to be used on hot path or in general code.
func Retry(ctx context.Context, op Operation, opts ...Option) error {
	var cfg Config

	for _, o := range opts {
		cfg = o(cfg)
	}

	b := newBackOff(cfg)
	return execBackOff(ctx, b, op, nil)
}
