package pretty

import (
	"encoding/json"
	"fmt"

	"gopkg.in/yaml.v2"
)

const (
	prettyJSONPrefix = ""
	prettyJSONIndent = "    "
)

// MarshalJSON marshals to JSON in pretty format
func MarshalJSON(v interface{}) ([]byte, error) {
	data, err := json.MarshalIndent(v, prettyJSONPrefix, prettyJSONIndent)
	return data, err
}

// MarshalYAML marshals to YAML in pretty format
func MarshalYAML(v interface{}) ([]byte, error) {
	data, err := json.Marshal(v)
	if err != nil {
		return nil, err
	}

	var m yaml.MapSlice
	if err = yaml.Unmarshal(data, &m); err != nil {
		return nil, err
	}

	return yaml.Marshal(m)
}

// Marshaller describes interface for marshaling object to pretty format
type Marshaller interface {
	Marshal(v interface{}) ([]byte, error)
}

// JSONMarshaller implements Marshaller for JSON
type JSONMarshaller struct{}

// Marshal ...
func (m *JSONMarshaller) Marshal(v interface{}) ([]byte, error) {
	return MarshalJSON(v)
}

// YAMLMarshaller implements Marshaller for YAML
type YAMLMarshaller struct{}

// Marshal ...
func (m *YAMLMarshaller) Marshal(v interface{}) ([]byte, error) {
	return MarshalYAML(v)
}

// Format is the type for pretty formatting
type Format string

// Possible pretty formats
const (
	JSONFormat Format = "json"
	YAMLFormat Format = "yaml"
)

// New constructs marshaller
func New(format Format) Marshaller {
	switch format {
	case JSONFormat:
		return &JSONMarshaller{}
	case YAMLFormat:
		return &YAMLMarshaller{}
	default:
		panic(fmt.Sprintf("unknown pretty format: %s", format))
	}
}
