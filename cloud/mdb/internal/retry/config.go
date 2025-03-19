package retry

import (
	"time"

	"github.com/cenkalti/backoff/v4"
)

// Config for BackOff
type Config struct {
	// MaxRetries sets the allowed number of retries. Zero means infinity.
	MaxRetries uint64 `yaml:"max_retries"`
	// InitialInterval for timeout between tries
	InitialInterval time.Duration `yaml:"initial_interval,omitempty"`
	// TODO: write correct parsing for optional fields. We cannot use float64 directly because we need to know if value is set
	//RandomizationFactor optional.Float64 `yaml:"randomization_factor,omitempty"`
	// TODO: write correct parsing for optional fields. We cannot use float64 directly because we need to know if value is set
	//Multiplier          optional.Float64          `yaml:"multiplier,omitempty"`
	// MaxInterval for timeout between tries
	MaxInterval time.Duration `yaml:"max_interval,omitempty"`
	// MaxElapsedTime for retries. It is NOT checked while operation is executing.
	MaxElapsedTime time.Duration `yaml:"max_elapsed_time,omitempty"`

	// clock may be used in tests to make time go faster
	clock backoff.Clock
}

func DefaultConfig() Config {
	return Config{MaxRetries: 2}
}
