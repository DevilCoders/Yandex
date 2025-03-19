package ready

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../scripts/mockgen.sh Checker

// Checker is an interface for checking if something is ready using error return
type Checker interface {
	IsReady(ctx context.Context) error
}

type checkResult struct {
	Error error
}

func waitForReady(ctx context.Context, rc Checker, period time.Duration, ch chan<- checkResult) {
	// Close channel when we leave func
	defer close(ch)
	// Proc timer right off the bat
	timer := time.NewTimer(0)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}

			// Close channel and return
			return
		case <-timer.C:
			if err := rc.IsReady(ctx); err != nil {
				ch <- checkResult{Error: err}

				// Wait and check one more time
				timer.Reset(period)
				continue
			}

			ch <- checkResult{}
			// Close channel and return
			return
		}
	}
}

// Wait for whatever that implements ready.Checker to become ready
// period is how often to check whether the thing is ready
func Wait(ctx context.Context, rc Checker, et ErrorTester, period time.Duration) error {
	// Cancel goroutine when we no longer wait for its data
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()
	ch := make(chan checkResult)
	go waitForReady(ctx, rc, period, ch)

	// Keep last error
	var lastErr error
	for {
		res, ok := <-ch
		// Channel was closed (either because we succeeded or because context was closed)
		if !ok {
			// Check failed (most likely due to context timeout or cancellation), report last error
			if ctx.Err() != nil {
				return xerrors.Errorf("%s with last error: %w", ctx.Err(), lastErr)
			}

			return lastErr
		}

		// No error? Check succeeded!
		if res.Error == nil {
			return nil
		}

		// Error is permanent? Report it!
		if et.IsPermanentError(ctx, res.Error) {
			return res.Error
		}

		// Keep temporary error and try again
		lastErr = res.Error
	}
}

func WaitWithTimeout(ctx context.Context, timeout time.Duration, rc Checker, et ErrorTester, period time.Duration) error {
	ctx, cancel := context.WithTimeout(ctx, timeout)
	defer cancel()
	return Wait(ctx, rc, et, period)
}

type checkerFunc func(ctx context.Context) error

func (cf checkerFunc) IsReady(ctx context.Context) error {
	return cf(ctx)
}

// CheckerFunc converts compatible function to Checker
func CheckerFunc(f func(ctx context.Context) error) Checker {
	return checkerFunc(f)
}

// ErrorTester is used in Wait to test whether ready error is permanent or not
type ErrorTester interface {
	IsPermanentError(ctx context.Context, err error) bool
}

// DefaultErrorTester implements very basic ready error testing
type DefaultErrorTester struct {
	// Name of 'something' you readycheck. If none provided, 'something' is used.
	Name string
	// FailOnError determines whether we permanently fail on any error or not.
	FailOnError bool
	// L is logger used to log the error. If none provided, fmt will be used to log to stdout.
	L log.Logger
}

// IsPermanentError implements ErrorTester interface
func (det *DefaultErrorTester) IsPermanentError(ctx context.Context, err error) bool {
	if det.L != nil {
		name := "unknown"
		if det.Name != "" {
			name = det.Name
		}

		msg := fmt.Sprintf("Error while waiting for %q to become ready", name)
		ctxlog.Error(ctx, det.L, msg, log.Error(err))

	}

	return det.FailOnError
}

type errorTesterFunc func(ctx context.Context, err error) bool

func (etf errorTesterFunc) IsPermanentError(ctx context.Context, err error) bool {
	return etf(ctx, err)
}

// ErrorTesterFunc converts compatible function to ErrorTester
func ErrorTesterFunc(f func(ctx context.Context, err error) bool) ErrorTester {
	return errorTesterFunc(f)
}
