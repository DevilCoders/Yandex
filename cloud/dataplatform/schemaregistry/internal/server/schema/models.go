package schema

import (
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
)

type ParsedSchema interface {
	IsBackwardCompatible(ParsedSchema) error
	IsForwardCompatible(ParsedSchema) error
	IsFullCompatible(ParsedSchema) error
	Diff(prev ParsedSchema) error
	Format() string
	GetCanonicalValue() *domain.SchemaFile
}

type SchemaProvider interface {
	ParseSchema(format string, data []byte) (ParsedSchema, error)
}
