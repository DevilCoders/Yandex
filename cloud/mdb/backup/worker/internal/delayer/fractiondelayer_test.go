package delayer

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func defaultConfig() FractionDelayerConfig {
	return FractionDelayerConfig{
		Fractions: map[string]encodingutil.Duration{
			"0.01": encodingutil.FromDuration(5 * time.Second),
			"0.10": encodingutil.FromDuration(30 * time.Minute),
			"0.50": encodingutil.FromDuration(time.Hour),
		},
		DefaultDelay: encodingutil.FromDuration(3 * time.Hour),
		MinDelay:     encodingutil.FromDuration(time.Minute),
	}
}

func TestNextFractionDelayerFunc(t *testing.T) {
	type args struct {
		started time.Time
		timeout time.Duration
		cfg     FractionDelayerConfig
	}
	tests := []struct {
		name             string
		args             args
		expectedDelay    time.Duration
		expectedTimedOut bool
		expectedErr      error
	}{
		{
			name: "bad_delay_values",
			args: args{
				cfg: FractionDelayerConfig{
					MinDelay:     encodingutil.FromDuration(10 * time.Second),
					DefaultDelay: encodingutil.FromDuration(time.Second),
				},
			},
			expectedErr: xerrors.Errorf("unexpected min_delay < default_delay: 10000000000 < 1000000000"),
		},
		{
			name: "bad_chars",
			args: args{
				cfg: FractionDelayerConfig{
					Fractions: map[string]encodingutil.Duration{
						"0.01": encodingutil.FromDuration(time.Second),
						"s":    encodingutil.FromDuration(time.Hour),
					},
				},
			},
			expectedErr: xerrors.Errorf("strconv.ParseFloat: parsing \"s\": invalid syntax"),
		},
		{
			name: "min_delay",
			args: args{
				cfg:     defaultConfig(),
				started: time.Now().Add(-(time.Minute)),
				timeout: 10 * time.Hour,
			},
			expectedTimedOut: false,
			expectedDelay:    time.Minute,
		},
		{
			name: "second_fraction",
			args: args{
				cfg:     defaultConfig(),
				started: time.Now().Add(-(30 * time.Minute)),
				timeout: 10 * time.Hour,
			},
			expectedTimedOut: false,
			expectedDelay:    30 * time.Minute,
		},
		{
			name: "third_fraction",
			args: args{
				cfg:     defaultConfig(),
				started: time.Now().Add(-(4 * time.Hour)),
				timeout: 10 * time.Hour,
			},
			expectedTimedOut: false,
			expectedDelay:    time.Hour,
		},
		{
			name: "max_delay",
			args: args{
				cfg:     defaultConfig(),
				started: time.Now().Add(-(7 * time.Hour)),
				timeout: 10 * time.Hour,
			},
			expectedTimedOut: false,
			expectedDelay:    3 * time.Hour,
		},
		{
			name: "timed_out",
			args: args{
				cfg:     defaultConfig(),
				started: time.Now().Add(-(20 * time.Hour)),
				timeout: 10 * time.Hour,
			},
			expectedTimedOut: true,
			expectedDelay:    0,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			fu, err := NextFractionDelayerFunc(tt.args.cfg)
			if tt.expectedErr != nil {
				require.EqualError(t, err, tt.expectedErr.Error())
				return
			}
			require.NoError(t, err)

			delay, timedOut := fu(tt.args.started, tt.args.timeout)
			require.Equal(t, tt.expectedTimedOut, timedOut)
			require.Equal(t, tt.expectedDelay, delay)
		})
	}
}
