package encodingutil

import (
	"encoding/json"
	"time"
)

// Duration wraps time.Duration for JSON formatting (https://github.com/golang/go/issues/10275)
type Duration struct {
	time.Duration
}

// FromDuration is a convenience wrapper
func FromDuration(d time.Duration) Duration {
	return Duration{Duration: d}
}

// MarshalJSON implements the json.Marshaler interface
func (d Duration) MarshalJSON() ([]byte, error) {
	return []byte(`"` + d.String() + `"`), nil
}

// UnmarshalJSON implements the json.Unmarshaler interface
func (d *Duration) UnmarshalJSON(data []byte) error {
	var s string
	if err := json.Unmarshal(data, &s); err != nil {
		return err
	}

	v, err := time.ParseDuration(s)
	if err != nil {
		return err
	}

	d.Duration = v
	return nil
}

// MarshalYAML implements the yaml.Marshaler interface
func (d Duration) MarshalYAML() (interface{}, error) {
	return d.String(), nil
}

// UnmarshalYAML implements the yaml.Unmarshaler interface
func (d *Duration) UnmarshalYAML(unmarshal func(interface{}) error) error {
	var s string
	if err := unmarshal(&s); err != nil {
		return err
	}

	v, err := time.ParseDuration(s)
	if err != nil {
		return err
	}

	d.Duration = v
	return nil
}
