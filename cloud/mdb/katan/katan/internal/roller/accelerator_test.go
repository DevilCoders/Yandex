package roller_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/roller"
)

func TestStrictLinearAccelerator(t *testing.T) {
	acc3 := roller.StrictLinearAccelerator(3)
	tests := []struct {
		done    int
		running int
		want    bool
	}{
		{0, 0, true},
		{1, 0, true},
		{1, 1, true},
		{1, 2, false},
		{2, 0, true},
		{2, 1, true},
		{2, 2, true},
		{2, 3, false},
		{5, 3, false},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("For %d running and %d done. We can %+v accaleration", tt.running, tt.done, tt.want), func(t *testing.T) {
			require.Equal(t, tt.want, acc3(roller.RolloutStat{
				Done:    tt.done,
				Running: tt.running,
				Skipped: 0,
				Errors:  0,
			}))
		})
	}
	t.Run("shouldn't accelerate when have errors", func(t *testing.T) {
		require.False(t, acc3(roller.RolloutStat{
			Done:    1,
			Running: 1,
			Skipped: 0,
			Errors:  1,
		}))
	})
	t.Run("shouldn't accelerate when for skipped host", func(t *testing.T) {
		require.False(t, acc3(roller.RolloutStat{
			Done:    3,
			Running: 1,
			Skipped: 3,
			Errors:  0,
		}))
	})

	for _, badParallelism := range []int{-100, 0} {
		t.Run(fmt.Sprintf("panics on non positive parallelism: %d", badParallelism), func(t *testing.T) {
			require.Panics(t, func() { roller.StrictLinearAccelerator(badParallelism) })
		})
	}
}
