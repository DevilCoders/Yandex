package json

import (
	js "encoding/json"

	"github.com/santhosh-tekuri/jsonschema/v5"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/schema"
)

func GetParsedSchema(data []byte) (schema.ParsedSchema, error) {
	compiler := jsonschema.NewCompiler()
	compiler.Draft = jsonschema.Draft2020
	sc, _ := compiler.Compile("https://json-schema.org/draft/2020-12/schema")
	var val interface{}
	if err := js.Unmarshal(data, &val); err != nil {
		return nil, err
	}
	if err := sc.Validate(val); err != nil {
		return nil, err
	}
	return &Schema{data: data}, nil
}
