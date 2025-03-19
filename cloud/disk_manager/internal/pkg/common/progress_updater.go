package common

import (
	"context"
	"sync"
	"time"
)

func ProgressUpdater(
	ctx context.Context,
	period time.Duration,
	update func(context.Context) error,
) (func(), <-chan error) {

	var cancelCtx func()
	ctx, cancelCtx = context.WithCancel(ctx)

	errors := make(chan error, 1)

	var wg sync.WaitGroup
	wg.Add(1)

	go func() {
		defer wg.Done()
		defer close(errors)

		for {
			select {
			case <-time.After(period):
			case <-ctx.Done():
				errors <- ctx.Err()
				return
			}

			err := update(ctx)
			if err != nil {
				errors <- err
				return
			}
		}
	}()

	return func() {
		cancelCtx()
		wg.Wait()
	}, errors
}
