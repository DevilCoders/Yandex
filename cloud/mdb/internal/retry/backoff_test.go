package retry

import (
	"context"
	"errors"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestBackOff(t *testing.T) {
	expectedErr := errors.New("foo")

	var inputs = []struct {
		Name             string
		Config           Config
		Op               Operation
		ExpectedErr      error
		ExpectedAttempts int
	}{
		{
			Name:             "Noop",
			Op:               func() error { return nil },
			ExpectedAttempts: 1,
		},
		{
			Name:             "MaxRetries1",
			Config:           Config{MaxRetries: 1},
			Op:               func() error { return expectedErr },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 2,
		},
		{
			Name:             "MaxRetries2",
			Config:           Config{MaxRetries: 2},
			Op:               func() error { return expectedErr },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 3,
		},
		{
			Name:             "MaxElapsedTime1Nanosecond",
			Config:           Config{MaxElapsedTime: time.Nanosecond},
			Op:               func() error { return expectedErr },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 1,
		},
		{
			Name:             "MaxElapsedTime1Second",
			Config:           Config{MaxElapsedTime: time.Second},
			Op:               func() error { return expectedErr },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 2,
		},
		{
			Name:             "MaxRetries1Permanent",
			Config:           Config{MaxRetries: 1},
			Op:               func() error { return Permanent(expectedErr) },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 1,
		},
		{
			Name:             "MaxRetries100500Permanent",
			Config:           Config{MaxRetries: 100500},
			Op:               func() error { return Permanent(expectedErr) },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 1,
		},
		{
			Name:             "MaxRetries1MaxElapsedTime1Nanosecond",
			Config:           Config{MaxRetries: 1, MaxElapsedTime: time.Nanosecond},
			Op:               func() error { return expectedErr },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 1,
		},
		{
			Name:             "100Retries",
			Config:           Config{MaxRetries: 100, MaxInterval: time.Nanosecond},
			Op:               func() error { return expectedErr },
			ExpectedErr:      expectedErr,
			ExpectedAttempts: 101,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			b := New(input.Config)
			assert.NotNil(t, b)

			var attempts int
			var notifies int
			err := b.RetryNotify(
				context.Background(),
				func() error {
					attempts++
					return input.Op()
				},
				func(err error, _ time.Duration) {
					notifies++
					assert.True(t, xerrors.Is(err, input.ExpectedErr))
				},
			)

			assert.True(t, xerrors.Is(err, input.ExpectedErr))
			assert.Equal(t, input.ExpectedAttempts, attempts)
			assert.Equal(t, input.ExpectedAttempts-1, notifies) // Notifies are sent for errors only
		})
	}
}

type testClock struct {
	now time.Time
}

func (t *testClock) Now() time.Time {
	return t.now
}

func TestMaxElapsedTime(t *testing.T) {
	expectedErr := errors.New("foo")

	var inputs = []struct {
		Name                      string
		MaxElapsedTime            time.Duration
		TimeStep                  time.Duration
		ExpectedAttempts          int
		ExpectedToRetryInfinitely bool
		ExpectedErr               error
	}{
		{
			Name:                      "RetryNoMoreThanMaxElapsedTime",
			MaxElapsedTime:            10 * time.Minute,
			TimeStep:                  time.Minute,
			ExpectedToRetryInfinitely: false,
			ExpectedAttempts:          10,
			ExpectedErr:               expectedErr,
		},
		{
			Name:                      "RetryInfinitely",
			MaxElapsedTime:            0,
			TimeStep:                  time.Hour,
			ExpectedToRetryInfinitely: true,
			ExpectedErr:               context.Canceled,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			clock := testClock{now: time.Now()}
			cfg := Config{
				MaxElapsedTime: input.MaxElapsedTime,
				MaxInterval:    time.Nanosecond,
				clock:          &clock,
			}
			b := New(cfg)
			assert.NotNil(t, b)

			attempts := 0
			attemptsLimit := 1000 // more is infinity
			cancelled := false
			ctx, cancel := context.WithCancel(context.Background())
			defer cancel()

			err := b.Retry(ctx, func() error {
				attempts += 1
				if attempts >= attemptsLimit {
					cancelled = true
					cancel()
				}
				clock.now = clock.now.Add(input.TimeStep)
				return expectedErr
			})

			assert.True(t, xerrors.Is(err, input.ExpectedErr))
			assert.Equal(t, input.ExpectedToRetryInfinitely, cancelled)
			if !cancelled {
				assert.Equal(t, input.ExpectedAttempts, attempts)
			}
		})
	}
}

func TestRace(t *testing.T) {
	const (
		retriesCount    = 10
		goroutinesCount = 100
	)

	b := New(
		Config{
			MaxRetries:      retriesCount,
			InitialInterval: time.Millisecond * 50,
			MaxInterval:     time.Millisecond * 50,
		},
	)
	var wg sync.WaitGroup
	wg.Add(goroutinesCount)
	for i := 0; i < goroutinesCount; i++ {
		go func() {
			_ = b.Retry(
				context.Background(),
				func() error {
					return errors.New("foo")
				},
			)

			wg.Done()
		}()
	}

	wg.Wait()
}
