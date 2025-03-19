package pretty

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"

	"gopkg.in/yaml.v2"
)

var ErrUnknown = errors.New("unknown format")

func Print(data []byte, format string) error {
	var out string

	switch format {
	case "json":

		var pretty bytes.Buffer
		if err := json.Indent(&pretty, data, "", "\t"); err != nil {
			return err
		}
		out = pretty.String()

	case "yaml":

		var jsonObj interface{}
		if err := yaml.Unmarshal(data, &jsonObj); err != nil {
			return err
		}

		byteOut, err := yaml.Marshal(jsonObj)
		if err != nil {
			return err
		}
		out = string(byteOut)

	default:
		return ErrUnknown
	}

	fmt.Println(out)

	return nil
}
