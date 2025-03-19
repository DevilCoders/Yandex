package secret

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/internal/config"
)

const maskedString = "xxx"

// String protects value from accidental exposure. It is a struct with private attribute because
// that way the value is hidden from reflection.
type String struct {
	value string
}

// NewString constructs secret value of type string
func NewString(v string) String {
	return String{value: v}
}

// String returns masked string
func (s String) String() string {
	return maskedString
}

// Unmask returns real value
func (s String) Unmask() string {
	return s.value
}

// UnmarshalJSON allows parsing secret from JSON
func (s *String) UnmarshalJSON(b []byte) error {
	return json.Unmarshal(b, &s.value)
}

// MarshalJSON hides secret as pure string
func (s String) MarshalJSON() ([]byte, error) {
	return json.Marshal(s.String())
}

// UnmarshalYAML allows parsing secret from YAML
func (s *String) UnmarshalYAML(unmarshal func(interface{}) error) error {
	return unmarshal(&s.value)
}

// MarshalYAML hides secret as pure string
func (s String) MarshalYAML() (interface{}, error) {
	return s.String(), nil
}

func (s *String) FromEnv(name string) bool {
	return config.LoadEnvToString(name, &s.value)
}
