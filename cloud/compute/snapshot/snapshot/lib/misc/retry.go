package misc

import (
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"

	"golang.org/x/xerrors"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

const (
	retryCount      = 10
	retryStartDelay = 100 * time.Millisecond
	maxRetryDelay   = 60 * time.Second
)

// Retry is a decorator for retriable errors processing.
func Retry(ctx context.Context, name string, do func() error) error {
	return RetryExtended(ctx, name, RetryParams{}, do)
}

type RetryParams = struct {
	RetryCount    int
	MaxRetryDelay time.Duration
	RetryTimeout  time.Duration
}

// Same as Retry but retries until specified timeout, using max retry allowed and custom retry count
// By default: 60 seconds at max on each iteration,10 iteration
func RetryExtended(ctx context.Context, name string, params RetryParams, do func() error) error {
	if params.MaxRetryDelay == 0 {
		params.MaxRetryDelay = maxRetryDelay
	}
	if params.RetryCount == 0 && params.RetryTimeout == 0 {
		params.RetryCount = retryCount
	}
	var timeout <-chan time.Time
	if params.RetryTimeout != 0 {
		timeout = time.After(params.RetryTimeout)
	}
	infiniteRetry := params.RetryCount == 0

	ctx = log.WithLogger(ctx, log.G(ctx).With(zap.String("retry_name", name)))
	if err := do(); err != ErrInternalRetry {
		return err
	}
	for i, delay := 1, retryStartDelay; i <= params.RetryCount || infiniteRetry; i++ {
		select {
		case <-time.After(delay):
			log.G(ctx).Debug("Retry", zap.Int("attempt", i))
			delay = retryDelay(delay, params.MaxRetryDelay)
		case <-timeout:
			log.G(ctx).Error("Retry timeout reached")
			return xerrors.Errorf("retry timeout reached! name=%s", name)
		case <-ctx.Done():
			log.G(ctx).Error("Retry was canceled due to <-ctx.Done() ")
			return ctx.Err()
		}
		if err := do(); !xerrors.Is(err, ErrInternalRetry) {
			if isFatalError(err) {
				log.G(ctx).Panic("Fatal error while DB transaction", zap.Error(err))
			}
			return err
		}
	}
	log.G(ctx).Error("Retry failed")
	return xerrors.Errorf("retry failed! name=%s", name)
}

func isFatalError(err error) bool {
	return xerrors.Is(err, table.ErrSessionPoolOverflow)
}

func retryDelay(currentDelay, maxDelay time.Duration) time.Duration {
	if currentDelay < maxDelay {
		currentDelay *= 2
	}
	if currentDelay > maxDelay {
		return maxDelay
	}
	return currentDelay
}
