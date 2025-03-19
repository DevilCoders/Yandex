package encodingutil

import (
	"encoding/json"
	"time"
)

const dateLayout = time.RFC3339

// Duration wraps time.Time
type DateTime struct {
	time.Time
}

// FromTime is a convenience wrapper
func DateTimeFromTime(d time.Time) DateTime {
	return DateTime{Time: d}
}

func (d DateTime) String() string {
	return d.Format(dateLayout)
}

// MarshalJSON implements the json.Marshaler interface
func (d DateTime) MarshalJSON() ([]byte, error) {
	return []byte(`"` + d.String() + `"`), nil
}

// UnmarshalJSON implements the json.Unmarshaler interface
func (d *DateTime) UnmarshalJSON(data []byte) error {
	var s string
	if err := json.Unmarshal(data, &s); err != nil {
		return err
	}

	v, err := time.Parse(dateLayout, s)
	if err != nil {
		return err
	}

	d.Time = v
	return nil
}

// MarshalYAML implements the yaml.Marshaler interface
func (d DateTime) MarshalYAML() (interface{}, error) {
	return d.String(), nil
}

// UnmarshalYAML implements the yaml.Unmarshaler interface
func (d *DateTime) UnmarshalYAML(unmarshal func(interface{}) error) error {
	var s string
	if err := unmarshal(&s); err != nil {
		return err
	}

	v, err := time.Parse(dateLayout, s)
	if err != nil {
		return err
	}

	d.Time = v
	return nil
}
