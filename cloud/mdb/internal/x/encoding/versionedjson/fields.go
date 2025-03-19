package versionedjson

import (
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type versionedFieldsPair struct {
	Key            string
	VersionField   *versionField
	VersionedField *versionField
}

type versionField struct {
	FieldName     string
	JSONFieldName string
	Key           string
	AllowMissing  bool
	Version       bool
	Pair          *versionedFieldsPair
}

func parseVersionField(field reflect.StructField) (*versionField, error) {
	v, ok := field.Tag.Lookup("version")
	if !ok {
		return nil, nil
	}

	flags := strings.Split(v, ",")
	if len(flags) < 2 {
		return nil, xerrors.Errorf("version tag must have at least 2 arguments but field %q has %d: %q", field.Name, len(flags), v)
	}

	if len(flags) > 3 {
		return nil, xerrors.Errorf("version tag must have at most 3 arguments but field %q has %d: %q", field.Name, len(flags), v)
	}

	if flags[1] == "" {
		return nil, xerrors.Errorf("version key tag for field %q is empty: %q", field.Name, v)
	}

	vt := versionField{FieldName: field.Name, JSONFieldName: jsonFieldName(field), Key: flags[1]}
	switch flags[0] {
	case "version":
		vt.Version = true
	case "versioned":
	default:
		return nil, xerrors.Errorf("version type tag must be either \"version\" or \"versioned\" but %q field has %q", field.Name, flags[0])
	}

	if len(flags) == 3 {
		switch flags[2] {
		case "allow_missing":
			vt.AllowMissing = true
		default:
			// TODO: test
			return nil, xerrors.Errorf("version missing tag must be either \"allow_missing\" or not exist at all but %q field has %q", field.Name, flags[2])
		}
	}

	return &vt, nil
}

func jsonFieldName(field reflect.StructField) (name string) {
	defer func() {
		name = strings.ToLower(name)
	}()

	v, ok := field.Tag.Lookup("json")
	if !ok {
		return field.Name
	}

	flags := strings.Split(v, ",")
	if len(flags) < 1 {
		return field.Name
	}

	if flags[0] == "" {
		return field.Name
	}

	return flags[0]
}
