package protobuf

import (
	"errors"

	"github.com/gofrs/uuid"
	"google.golang.org/protobuf/reflect/protoregistry"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/schema"
)

const protobufFormat = "FORMAT_PROTOBUF"

type Schema struct {
	*protoregistry.Files
	isValid bool
	data    []byte
}

func (s *Schema) Format() string {
	return protobufFormat
}

func (s *Schema) GetCanonicalValue() *domain.SchemaFile {
	id, _ := uuid.NewV4()
	return &domain.SchemaFile{
		ID:     id.String(),
		Types:  getAllMessages(s.Files),
		Data:   s.data,
		Fields: getAllFields(s.Files),
	}
}

func (s *Schema) verify(against schema.ParsedSchema) (*Schema, error) {
	prev, ok := against.(*Schema)
	if against.Format() != protobufFormat && !ok {
		return prev, errors.New("different schema formats")
	}
	return prev, nil
}

// IsBackwardCompatible checks backward compatibility against given schema
// Allowed changes: field addition
// Disallowed changes: field type change, tag number change, label change, field deletion
func (s *Schema) IsBackwardCompatible(against schema.ParsedSchema) error {
	prev, err := s.verify(against)
	if err != nil {
		return err
	}
	return compareSchemas(s.Files, prev.Files, backwardCompatibility)
}

// IsForwardCompatible for protobuf forward compatible is same as backward compatible
// Allowed changes: field addition, field deletion given tag number marked as reserved
// Disallowed changes: field type change, tag number change, label change
func (s *Schema) IsForwardCompatible(against schema.ParsedSchema) error {
	prev, err := s.verify(against)
	if err != nil {
		return err
	}
	return compareSchemas(s.Files, prev.Files, forwardCompatibility)
}

// IsFullCompatible for protobuf forward compatible is same as backward compatible
func (s *Schema) IsFullCompatible(against schema.ParsedSchema) error {
	prev, err := s.verify(against)
	if err != nil {
		return err
	}
	return compareSchemas(s.Files, prev.Files, fullCompatibility)
}

func (s *Schema) Diff(against schema.ParsedSchema) error {
	prev, err := s.verify(against)
	if err != nil {
		return err
	}
	return compareSchemas(s.Files, prev.Files, nil)
}
