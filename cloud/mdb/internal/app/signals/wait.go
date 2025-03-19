package signals

import (
	"context"
	"os"
	"os/signal"
	"syscall"
)

var stopSignals = []os.Signal{syscall.SIGINT, syscall.SIGTERM}

// WaitForStop waits for either SIGINT or SIGTERM
func WaitForStop() {
	<-prepareWait()
}

func prepareWait() <-chan os.Signal {
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, stopSignals...)
	return sigs
}

func waitForStopTestable(ready chan<- struct{}) {
	sigs := prepareWait()
	close(ready)
	<-sigs
}

// WithCancelOnSignal cancels provided context when either SIGINT or SIGTERM is received
func WithCancelOnSignal(ctx context.Context) context.Context {
	ctx, cancel := context.WithCancel(ctx)
	go func() {
		WaitForStop()
		cancel()
	}()

	return ctx
}

func withCancelOnSignalTestable(ctx context.Context, ready chan<- struct{}) context.Context {
	ctx, cancel := context.WithCancel(ctx)
	go func() {
		waitForStopTestable(ready)
		cancel()
	}()

	return ctx
}
