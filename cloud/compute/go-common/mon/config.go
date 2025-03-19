package mon

import (
	"time"
)

// NOTE: seems a good idea to add UnmarshalText to time.Duration in upstream

// Duration is a just like plain time.Duration but supports UnmarshalText
type Duration time.Duration

// UnmarshalText implements encoding.TextUnmarshaler
func (d *Duration) UnmarshalText(text []byte) error {
	timeout, err := time.ParseDuration(string(text))
	if err != nil {
		return err
	}

	(*d) = Duration(timeout)
	return nil
}

// Config is a configuration section
// Other packages are suggested to embed into their configuration struct
type Config struct {
	Timeout Duration `toml:"timeout" yaml:"timeout" json:"timeout"`
}

// RepositoryOptions converts configuration to []RepositoryOption
func (c *Config) RepositoryOptions() []RepositoryOption {
	var opts []RepositoryOption
	if c.Timeout != 0 {
		opts = append(opts, WithTimeout(time.Duration(c.Timeout)))
	}

	return opts
}
