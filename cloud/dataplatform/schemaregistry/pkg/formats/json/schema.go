package json

import (
	"errors"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/schema"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const jsonFormat = "FORMAT_JSON"

type Schema struct {
	data []byte
}

func (s *Schema) Format() string {
	return jsonFormat
}

func (s *Schema) GetCanonicalValue() *domain.SchemaFile {
	id, _ := uuid.NewV4()
	return &domain.SchemaFile{
		ID:   id.String(),
		Data: s.data,
	}
}

// IsBackwardCompatible checks backward compatibility against given schema
func (s *Schema) IsBackwardCompatible(against schema.ParsedSchema) error {
	return errors.New("not implemented")
}

// IsForwardCompatible checks backward compatibility against given schema
func (s *Schema) IsForwardCompatible(against schema.ParsedSchema) error {
	return against.IsBackwardCompatible(s)
}

// IsFullCompatible checks for forward compatibility
func (s *Schema) IsFullCompatible(against schema.ParsedSchema) error {
	forwardErr := s.IsForwardCompatible(against)
	backwardErr := s.IsBackwardCompatible(against)
	return xerrors.Errorf("forward: %v, backward: %w", forwardErr, backwardErr)
}

func (s *Schema) Diff(against schema.ParsedSchema) error {
	return nil
}
