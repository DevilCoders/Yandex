package printer

import (
	"encoding/json"
	"fmt"

	"gopkg.in/yaml.v2"
)

// YAML prints the data as YAML.
func YAML(data interface{}) error {
	return Text(data, "yaml")
}

// JSON prints the data as JSON.
func JSON(data interface{}) error {
	return Text(data, "json")
}

// PrettyJSON prints the data as pretty JSON.
func PrettyJSON(data interface{}) error {
	return Text(data, "prettyjson")
}

// Text prints the data for a given format
func Text(data interface{}, format string) (err error) {
	var output []byte
	switch format {
	case "yaml":
		output, err = yaml.Marshal(data)
	case "json":
		output, err = json.Marshal(data)
	case "prettyjson":
		output, err = json.MarshalIndent(data, "", "\t")
	default:
		return fmt.Errorf("unknown format: %v", format)
	}

	if err != nil {
		return err
	}

	fmt.Println(string(output))
	return nil
}
