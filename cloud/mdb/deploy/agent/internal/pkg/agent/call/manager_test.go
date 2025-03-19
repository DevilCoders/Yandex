package call_test

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/call"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/salt"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func inDeadlineProduceNChanges(ctx context.Context, t *testing.T, changes <-chan salt.Change, expectElements int) []salt.Change {
	ret := make([]salt.Change, 0)
	for {
		select {
		case c := <-changes:
			ret = append(ret, c)
			if len(ret) == expectElements {
				return ret
			}
		case <-ctx.Done():
			t.Fatalf("Within deadline chanel produce only %d elements while %d required", len(ret), expectElements)
		}
	}
}

func TestManagerRun(t *testing.T) {
	cfg := call.DefaultConfig()
	cfg.StopTimeout.Duration = time.Nanosecond
	t.Run("Simple command", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		m := call.New(&nop.Logger{}, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/echo",
			Args: []string{"-n", "foo"},
		}, false)
		c := inDeadlineProduceNChanges(ctx, t, m.Changes(), 1)[0]
		require.Positivef(t, c.PID, "PID should be a positive")
		require.Truef(t, c.Result.Done(), "Change result is Done()")
		require.Equal(t, salt.Result{
			ExitCode:   0,
			Error:      nil,
			Stdout:     "foo",
			Stderr:     "",
			FinishedAt: c.Result.FinishedAt,
		}, c.Result)
	})

	t.Run("Run 3 commands", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		m := call.New(&nop.Logger{}, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/echo",
			Args: []string{"-n", "bar"},
		}, false)
		m.Run(ctx, salt.Job{
			ID:   "jid-2",
			Name: "/bin/echo",
			Args: []string{"42"},
		}, false)
		m.Run(ctx, salt.Job{
			ID:   "jid-3",
			Name: "/bin/echo",
			Args: []string{"42"},
		}, false)

		changes := inDeadlineProduceNChanges(ctx, t, m.Changes(), 3)
		for _, c := range changes {
			require.NoError(t, c.Result.Error)
			require.Equal(t, 0, c.Result.ExitCode)
		}
	})

	t.Run("Command that fail", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		m := call.New(&nop.Logger{}, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "non-existed-command-expect-that-really-not-exits-on-distbuild",
			Args: nil,
		}, false)
		c := inDeadlineProduceNChanges(ctx, t, m.Changes(), 1)[0]
		require.Error(t, c.Result.Error)
		require.Equal(t, -1, c.Result.ExitCode)
	})

	t.Run("killed command", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		lg, err := zap.New(zap.CLIConfig(log.DebugLevel))
		require.NoError(t, err)
		m := call.New(lg, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/sh",
			Args: []string{"-c", "/bin/sleep 0.001 && /bin/kill $$"},
		}, false)
		c := inDeadlineProduceNChanges(ctx, t, m.Changes(), 1)[0]
		require.Error(t, c.Result.Error)
	})

	t.Run("run command with progress", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		lg, err := zap.New(zap.CLIConfig(log.DebugLevel))
		require.NoError(t, err)
		m := call.New(lg, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/echo",
			Args: []string{"-n", "bar"},
		}, true)
		changes := inDeadlineProduceNChanges(ctx, t, m.Changes(), 2)
		require.NoError(t, changes[0].Result.Error)
		require.False(t, changes[0].Result.Done())
		require.Regexp(t, `.*started. its pid is \d+`, changes[0].Progress)
	})
}

type testLogger struct {
	nop.Logger
	warns []string
}

func (l *testLogger) Warnf(format string, args ...interface{}) {
	l.warns = append(l.warns, format)
}

func TestManager_Shutdown(t *testing.T) {
	cfg := call.DefaultConfig()
	cfg.StopTimeout.Duration = time.Nanosecond

	t.Run("For a finished command", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		logger := &testLogger{}
		m := call.New(logger, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/echo",
			Args: []string{"-n", "foo"},
		}, false)
		inDeadlineProduceNChanges(ctx, t, m.Changes(), 1)

		m.Shutdown()
		require.Empty(t, logger.warns, "No warnings should be written")
	})

	t.Run("Kill multiply commands", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		lg, err := zap.New(zap.CLIConfig(log.DebugLevel))
		require.NoError(t, err)
		m := call.New(lg, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/sleep",
			Args: []string{"600"},
		}, false)
		m.Run(ctx, salt.Job{
			ID:   "jid-2",
			Name: "/bin/sleep",
			Args: []string{"700"},
		}, false)
		m.Run(ctx, salt.Job{
			ID:   "jid-3",
			Name: "/bin/sleep",
			Args: []string{"800"},
		}, false)

		m.Shutdown()
		inDeadlineProduceNChanges(ctx, t, m.Changes(), 3)
	})

	t.Run("Kill command with children", func(t *testing.T) {
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		lg, err := zap.New(zap.CLIConfig(log.DebugLevel))
		require.NoError(t, err)
		m := call.New(lg, cfg)

		m.Run(ctx, salt.Job{
			ID:   "jid-1",
			Name: "/bin/sh",
			Args: []string{"-c", "tail -f /dev/zero | grep 42"},
		}, false)
		m.Shutdown()
		inDeadlineProduceNChanges(ctx, t, m.Changes(), 1)
	})
}
