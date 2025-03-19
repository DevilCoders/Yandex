package signals

import (
	"context"
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
)

func TestWaitForStop(t *testing.T) {
	for _, d := range stopSignals {
		t.Run(fmt.Sprintf("Signal_%s", d), func(t *testing.T) {
			// Prepare to wait the signal
			ready := make(chan struct{})
			stopped := waitForStop(ready)
			_, ok := <-ready
			require.False(t, ok)

			// Send signal
			p, err := os.FindProcess(os.Getpid())
			require.NoError(t, err)
			require.NoError(t, p.Signal(d))

			// Check if test succeeded
			select {
			case <-time.After(time.Second * 5):
				t.Fatal("test timed out")
			case _, ok := <-stopped:
				require.False(t, ok)
			}
		})
	}
}

func waitForStop(ready chan<- struct{}) <-chan struct{} {
	stopped := make(chan struct{})
	go func() {
		waitForStopTestable(ready)
		close(stopped)
	}()

	return stopped
}

func TestWithCancelOnSignal(t *testing.T) {
	for _, d := range stopSignals {
		t.Run(fmt.Sprintf("Signal_%s", d), func(t *testing.T) {
			// Prepare to wait the signal
			ctx := context.Background()
			ready := make(chan struct{})
			ctx = withCancelOnSignalTestable(ctx, ready)
			_, ok := <-ready
			require.False(t, ok)

			// Send signal
			p, err := os.FindProcess(os.Getpid())
			require.NoError(t, err)
			require.NoError(t, p.Signal(d))

			// Check if test succeeded
			select {
			case <-time.After(time.Second * 5):
				t.Fatal("test timed out")
			case <-ctx.Done():
			}
		})
	}
}
