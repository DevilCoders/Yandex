package provider

import (
	"errors"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/schema"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/pkg/formats/json"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/pkg/formats/protobuf"
)

type parseFn func([]byte) (schema.ParsedSchema, error)

type SchemaProvider struct {
	mapper map[string]parseFn
}

func (s *SchemaProvider) ParseSchema(format string, data []byte) (schema.ParsedSchema, error) {
	fn, ok := s.mapper[format]
	if ok {
		return fn(data)
	}
	return nil, errors.New("unknown schema")
}

func NewSchemaProvider() *SchemaProvider {
	mp := make(map[string]parseFn)
	mp["FORMAT_PROTOBUF"] = protobuf.GetParsedSchema
	mp["FORMAT_JSON"] = json.GetParsedSchema
	return &SchemaProvider{
		mapper: mp,
	}
}
