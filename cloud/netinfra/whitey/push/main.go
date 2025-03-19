package push

import (
	"errors"
)

type Type int

const (
	UA Type = iota
)

func (p Type) String() string {
	return [...]string{
		"ua",
	}[p]
}

func (t *Type) UnmarshalYAML(unmarshal func(v interface{}) error) error {
	var value string

	if err := unmarshal(&value); err != nil {
		return err
	}

	switch value {
	case "ua":
		*t = UA
	default:
		return errors.New("Unknown push type: " + value)
	}

	return nil
}

type Data struct {
	Url  string `yaml:"url"`
	Type Type   `yaml:"type"`
}
