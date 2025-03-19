package delayer

import (
	"sort"
	"strconv"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FractionDelayerConfig struct {
	Fractions    map[string]encodingutil.Duration `json:"fractions" yaml:"fractions"`
	MinDelay     encodingutil.Duration            `json:"min_delay" yaml:"min_delay"`
	DefaultDelay encodingutil.Duration            `json:"default_delay" yaml:"default_delay"`
}

func DefaultFractionDelayerConfig() FractionDelayerConfig {
	return FractionDelayerConfig{
		Fractions: map[string]encodingutil.Duration{
			"0.05": encodingutil.FromDuration(time.Minute),
			"0.1":  encodingutil.FromDuration(5 * time.Minute),
		},
		DefaultDelay: encodingutil.FromDuration(10 * time.Minute),
		MinDelay:     encodingutil.FromDuration(time.Minute),
	}
}

type NextDelayFunc func(since time.Time, timeout time.Duration) (delay time.Duration, timedOut bool)

type FractionDelay struct {
	Fraction float32
	Delay    time.Duration
}

func NextFractionDelayerFunc(cfg FractionDelayerConfig) (NextDelayFunc, error) {
	if cfg.MinDelay.Duration > cfg.DefaultDelay.Duration {
		return nil, xerrors.Errorf("unexpected min_delay < default_delay: %d < %d",
			cfg.MinDelay.Duration, cfg.DefaultDelay.Duration)
	}

	fractions := make([]FractionDelay, len(cfg.Fractions))

	keys := make([]string, 0, len(cfg.Fractions))
	for k := range cfg.Fractions {
		keys = append(keys, k)
	}
	sort.Strings(keys)

	for i := range keys {
		fraction, err := strconv.ParseFloat(keys[i], 32)
		if err != nil {
			return nil, err
		}
		fractions[i].Fraction = float32(fraction)
		fractions[i].Delay = cfg.Fractions[keys[i]].Duration
		i++
	}

	return func(since time.Time, timeout time.Duration) (time.Duration, bool) {
		now := time.Now()
		if since.Add(timeout).Before(now) {
			return 0, true
		}
		leftFraction := 1 - float32(timeout-now.Sub(since))/float32(timeout)

		for i := range fractions {
			if leftFraction < fractions[i].Fraction {
				if fractions[i].Delay > cfg.MinDelay.Duration {
					return fractions[i].Delay, false
				}
				return cfg.MinDelay.Duration, false
			}
		}
		return cfg.DefaultDelay.Duration, false
	}, nil
}
