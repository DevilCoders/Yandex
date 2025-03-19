package retry

import (
	"context"
	"sync"
	"time"

	"github.com/cenkalti/backoff/v4"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// BackOff is a thread-safe reusable object for retrying operations
type BackOff struct {
	cfg Config

	pool sync.Pool
}

// New constructs thread-safe BackOff object
func New(cfg Config) *BackOff {
	f := func() interface{} {
		return newBackOff(cfg)
	}

	return &BackOff{
		cfg:  cfg,
		pool: sync.Pool{New: f},
	}
}

func newBackOff(cfg Config) backoff.BackOff {
	expb := backoff.NewExponentialBackOff()
	if cfg.clock != nil {
		expb.Clock = cfg.clock
	}

	if cfg.InitialInterval != 0 {
		expb.InitialInterval = cfg.InitialInterval
	}

	if cfg.MaxInterval != 0 {
		expb.MaxInterval = cfg.MaxInterval
	}

	expb.MaxElapsedTime = cfg.MaxElapsedTime

	b := backoff.BackOff(expb)

	if cfg.MaxRetries != 0 {
		b = backoff.WithMaxRetries(b, cfg.MaxRetries)
	}

	return b
}

func execBackOff(ctx context.Context, b backoff.BackOff, op Operation, notify Notify) error {
	b.Reset()
	b = backoff.WithContext(b, ctx)
	return backoff.RetryNotify(op, b, notify)
}

func execBackOffWithCounter(ctx context.Context, b backoff.BackOff, op operationWithCounter, notify notifyWithCounter) error {
	var attempt int
	// Wrap operation with attempt counter
	baseOp := func() error {
		attempt++
		return op(attempt)
	}
	// Wrap notify callback with attempt counter
	baseNotify := func(err error, d time.Duration) {
		notify(err, d, attempt)
	}

	return execBackOff(ctx, b, baseOp, baseNotify)
}

// Retry given operation
func (b *BackOff) Retry(ctx context.Context, op Operation) error {
	return b.RetryNotify(ctx, op, nil)
}

// RetryNotify retries given operation and notifies callback
func (b *BackOff) RetryNotify(ctx context.Context, op Operation, notify Notify) error {
	obj := b.pool.Get()
	defer b.pool.Put(obj)
	return execBackOff(ctx, obj.(backoff.BackOff), op, notify)
}

// RetryWithLog retry and log attempts as Warning, fails as Error
func (b *BackOff) RetryWithLog(ctx context.Context, op Operation, msg string, l log.Logger) error {
	logOp := func(attempt int) error {
		if attempt > 1 {
			logCtx := ctxlog.WithFields(ctx, log.Int(attemptLogKey, attempt))
			ctxlog.Debugf(logCtx, l, "%s [retry]", msg)
		}

		return op()
	}

	obj := b.pool.Get()
	defer b.pool.Put(obj)

	err := execBackOffWithCounter(
		ctx,
		obj.(backoff.BackOff),
		logOp,
		func(err error, backoffDelay time.Duration, attempt int) {
			logCtx := ctxlog.WithFields(ctx, log.Int(attemptLogKey, attempt), log.Error(err))
			ctxlog.Warnf(logCtx, l, "%s [retry in %s]", msg, backoffDelay)
		},
	)
	if err != nil {
		ctxlog.Errorf(ctxlog.WithFields(ctx, log.Error(err)), l, "%s [retries exhausted]", msg)
	}

	return err
}

const attemptLogKey = "retry_attempt"

type (
	// Operation executing in Retry
	Operation            = backoff.Operation
	operationWithCounter func(attempt int) error
)

type (
	// Notify callback
	Notify            = backoff.Notify
	notifyWithCounter func(error, time.Duration, int)
)

// PermanentError indicates that the operation should not be retried
type PermanentError = backoff.PermanentError

// Permanent wraps an error with PermanentError
var Permanent = backoff.Permanent
