package functest

import (
	"context"

	"github.com/cenkalti/backoff/v4"
)

func retry(ctx context.Context, maxRetries uint64, op func() error) error {
	var b backoff.BackOff
	b = backoff.NewExponentialBackOff()
	b = backoff.WithContext(b, ctx)
	if maxRetries > 0 {
		b = backoff.WithMaxRetries(b, maxRetries)
	}
	return backoff.Retry(op, b)
}
