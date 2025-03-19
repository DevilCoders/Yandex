package godogutil

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
)

func TestFormatTestDuration(t *testing.T) {
	inputs := []struct {
		Dur      time.Duration
		Expected string
	}{
		{
			Expected: "0.00",
		},
		{
			Dur:      time.Nanosecond,
			Expected: "0.00",
		},
		{
			Dur:      time.Microsecond,
			Expected: "0.00",
		},
		{
			Dur:      time.Millisecond,
			Expected: "0.00",
		},
		{
			Dur:      time.Millisecond * 10,
			Expected: "0.01",
		},
		{
			Dur:      time.Millisecond * 100,
			Expected: "0.10",
		},
		{
			Dur:      time.Second / 2,
			Expected: "0.50",
		},
		{
			Dur:      time.Second,
			Expected: "1.00",
		},
		{
			Dur:      time.Minute,
			Expected: "60.00",
		},
		{
			Dur:      time.Hour,
			Expected: "3600.00",
		},
		{
			Dur:      time.Hour + time.Minute + time.Second + time.Millisecond*110,
			Expected: "3661.11",
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprint(input.Dur), func(t *testing.T) {
			require.Equal(t, input.Expected, formatTestDuration(input.Dur))
		})
	}
}
