package clickhouse

import (
	"context"
	"time"

	"github.com/cenkalti/backoff/v4"
)

type retrier struct {
	backoffOverride func() backoff.BackOff
}

func (s *retrier) retryInsert(ctx context.Context, op backoff.Operation) error {
	var bo backoff.BackOffContext
	if s.backoffOverride == nil {
		ebo := backoff.NewExponentialBackOff()
		ebo.InitialInterval = time.Millisecond * 200
		ebo.MaxInterval = time.Second * 5
		ebo.MaxElapsedTime = time.Second * 30
		ebo.RandomizationFactor = 0.25

		bo = backoff.WithContext(ebo, ctx)
	} else {
		bo = backoff.WithContext(s.backoffOverride(), ctx)
	}

	return backoff.Retry(op, bo)
}
