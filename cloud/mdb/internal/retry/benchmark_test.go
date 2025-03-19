package retry_test

import (
	"context"
	"errors"
	"testing"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/retry"
)

var ErrTest = errors.New("test error")

func Op() error {
	return ErrTest
}

func BenchmarkBackoff(b *testing.B) {
	cfg := retry.Config{
		MaxElapsedTime: time.Nanosecond,
	}

	backoff := retry.New(cfg)

	ctx := context.Background()

	b.Run("Retry", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_ = retry.Retry(ctx, Op, retry.WithMaxElapsedTime(cfg.MaxElapsedTime))
		}
	})

	b.Run("BackOff.Retry", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_ = backoff.Retry(ctx, Op)
		}
	})
}
