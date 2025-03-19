package timeutil

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"
)

func Test_splitHours(t *testing.T) {
	type args struct {
		start  time.Time
		finish time.Time
	}
	tests := []struct {
		name string
		args args
		want []time.Time
	}{
		{
			name: "without_split",
			args: args{
				start:  time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
				finish: time.Date(2022, 01, 01, 14, 50, 59, 0, time.UTC),
			},
			want: []time.Time{
				time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
				time.Date(2022, 01, 01, 14, 50, 59, 0, time.UTC),
			},
		},
		{
			name: "one_split",
			args: args{
				start:  time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
				finish: time.Date(2022, 01, 01, 15, 00, 10, 0, time.UTC),
			},
			want: []time.Time{
				time.Date(2022, 01, 01, 14, 00, 59, 0, time.UTC),
				time.Date(2022, 01, 01, 15, 00, 00, 0, time.UTC),
				time.Date(2022, 01, 01, 15, 00, 10, 0, time.UTC),
			},
		},
		{
			name: "multiple_split",
			args: args{
				start:  time.Date(2022, 01, 01, 22, 40, 00, 0, time.UTC),
				finish: time.Date(2022, 01, 02, 01, 50, 10, 0, time.UTC),
			},
			want: []time.Time{
				time.Date(2022, 01, 01, 22, 40, 00, 0, time.UTC),
				time.Date(2022, 01, 01, 23, 00, 00, 0, time.UTC),
				time.Date(2022, 01, 02, 00, 00, 00, 0, time.UTC),
				time.Date(2022, 01, 02, 01, 00, 00, 0, time.UTC),
				time.Date(2022, 01, 02, 01, 50, 10, 0, time.UTC),
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.want, SplitHours(tt.args.start, tt.args.finish))
		})
	}
}
