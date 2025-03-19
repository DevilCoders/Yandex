package chmodels

import "a.yandex-team.ru/cloud/mdb/internal/valid"

const formatSchemaNamePattern = "^[a-zA-Z0-9_-]+$"

type FormatSchema struct {
	ClusterID string
	Name      string
	Type      FormatSchemaType
	URI       string
}

type FormatSchemaType string

const (
	ProtobufFormatSchema  FormatSchemaType = "protobuf"
	CapNProtoFormatSchema FormatSchemaType = "capnproto"
)

// NewFormatSchemaNameValidator constructs validator for format schema names
func NewFormatSchemaNameValidator(pattern string) (*valid.StringComposedValidator, error) {
	return valid.NewStringComposedValidator(
		&valid.Regexp{
			Pattern: pattern,
			Msg:     "format schema name %q has invalid symbols",
		},
		&valid.StringLength{
			Min:         1,
			Max:         63,
			TooShortMsg: "format schema name %q is too short",
			TooLongMsg:  "format schema name %q is too long",
		},
	)
}

// MustFormatSchemaNameValidator constructs validator for format schema names. Panics on error.
func MustFormatSchemaNameValidator(pattern string) *valid.StringComposedValidator {
	v, err := NewFormatSchemaNameValidator(pattern)
	if err != nil {
		panic(err)
	}

	return v
}

var FormatSchemaNameValidator = MustFormatSchemaNameValidator(formatSchemaNamePattern)
