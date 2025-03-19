package katan_test

import (
	"context"
	"sync/atomic"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/katandb"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/katan"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestRunRollouts(t *testing.T) {
	cfg := katan.RolloutRunnerConfig{
		PendingInterval: time.Microsecond,
		Parallel:        100,
	}

	t.Run("stop on context close", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		katan.RunRollouts(
			ctx,
			func() (katandb.Rollout, error) {
				cancel()
				return katandb.Rollout{}, katandb.ErrNoDataFound
			},
			func(katandb.Rollout) {},
			cfg,
			&nop.Logger{},
		)

	})

	t.Run("ignore errors in pending handle", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()
		var cancelCallNum int

		katan.RunRollouts(
			ctx,
			func() (katandb.Rollout, error) {
				cancelCallNum++
				if cancelCallNum == 3 {
					cancel()
				}
				return katandb.Rollout{}, xerrors.New("test error")
			},
			func(katandb.Rollout) {},
			cfg,
			&nop.Logger{},
		)
	})

	t.Run("start each pending rollout it got", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		var pendingCalled int32
		var startCalled int32

		katan.RunRollouts(
			ctx,
			func() (katandb.Rollout, error) {
				pendingCalled++
				if pendingCalled > 33 {
					return katandb.Rollout{}, katandb.ErrNoDataFound
				}
				return katandb.Rollout{}, nil
			},
			func(katandb.Rollout) {
				atomic.AddInt32(&startCalled, 1)
				if atomic.LoadInt32(&startCalled) == 33 {
					cancel()
				}
			},
			cfg,
			&nop.Logger{},
		)
		require.Equal(t, int32(33), startCalled)
	})

	t.Run("don't start more then MaxRollouts", func(t *testing.T) {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()

		var startCalled int32
		katan.RunRollouts(
			ctx,
			func() (katandb.Rollout, error) {
				return katandb.Rollout{}, nil
			},
			func(katandb.Rollout) {
				atomic.AddInt32(&startCalled, 1)
			},
			katan.RolloutRunnerConfig{
				PendingInterval: time.Microsecond,
				Parallel:        100,
				MaxRollouts:     3,
			},
			&nop.Logger{},
		)
		require.Equal(t, int32(3), startCalled)

	})

	// here should be a test. Something like:
	//
	// t.Run("don't run more then parallel rollouts", func(t *testing.T) {
	// ....
	// with lots of magic here.
	//
	// But it looks like, that write that test is harder then write that runner
}
