package versionedjson

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestJSONFieldName(t *testing.T) {
	t.Run("Name", func(t *testing.T) {
		type S struct {
			F int `json:"name"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		require.Equal(t, "name", jsonFieldName(field))
	})

	t.Run("ToLower", func(t *testing.T) {
		type S struct {
			F int `json:"NaMe"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		require.Equal(t, "name", jsonFieldName(field))
	})

	t.Run("NameOmitEmpty", func(t *testing.T) {
		type S struct {
			F int `json:"name,omitempty"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		require.Equal(t, "name", jsonFieldName(field))
	})

	t.Run("NoName", func(t *testing.T) {
		type S struct {
			F int `json:",omitempty"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		require.Equal(t, "f", jsonFieldName(field))
	})

	t.Run("NoJSON", func(t *testing.T) {
		type S struct {
			F int
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		require.Equal(t, "f", jsonFieldName(field))
	})
}

func TestParseVersionField(t *testing.T) {
	t.Run("Version", func(t *testing.T) {
		type S struct {
			F int `version:"version,key"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.NoError(t, err)
		require.NotNil(t, v)
		require.Equal(t, versionField{FieldName: "F", JSONFieldName: "f", Key: "key", Version: true}, *v)
	})

	t.Run("Versioned", func(t *testing.T) {
		type S struct {
			F int `version:"versioned,key"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.NoError(t, err)
		require.NotNil(t, v)
		require.Equal(t, versionField{FieldName: "F", JSONFieldName: "f", Key: "key", Version: false}, *v)
	})

	t.Run("InvalidType", func(t *testing.T) {
		type S struct {
			F int `version:"foo,key"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version type tag must be either \"version\" or \"versioned\" but \"F\" field has \"foo\"")
		require.Nil(t, v)
	})

	t.Run("EmptyType", func(t *testing.T) {
		type S struct {
			F int `version:",key"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version type tag must be either \"version\" or \"versioned\" but \"F\" field has \"\"")
		require.Nil(t, v)
	})

	t.Run("InvalidAllowMissing", func(t *testing.T) {
		type S struct {
			F int `version:"version,key,stuff"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version missing tag must be either \"allow_missing\" or not exist at all but \"F\" field has \"stuff\"")
		require.Nil(t, v)
	})

	t.Run("EmptyAllowMissing", func(t *testing.T) {
		type S struct {
			F int `version:"version,key,"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version missing tag must be either \"allow_missing\" or not exist at all but \"F\" field has \"\"")
		require.Nil(t, v)
	})

	t.Run("JSONFieldName", func(t *testing.T) {
		type S struct {
			F int `json:"test_name" version:"version,key"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.NoError(t, err)
		require.NotNil(t, v)
		require.Equal(t, versionField{FieldName: "F", JSONFieldName: "test_name", Key: "key", Version: true}, *v)
	})

	t.Run("NoKey", func(t *testing.T) {
		type S struct {
			F int `version:"foo"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version tag must have at least 2 arguments but field \"F\" has 1: \"foo\"")
		require.Nil(t, v)
	})

	t.Run("TooManyArguments", func(t *testing.T) {
		type S struct {
			F int `version:"foo,key,allow_missing,stuff"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version tag must have at most 3 arguments but field \"F\" has 4: \"foo,key,allow_missing,stuff\"")
		require.Nil(t, v)
	})

	t.Run("EmptyKey", func(t *testing.T) {
		type S struct {
			F int `version:"version,"`
		}
		typ := reflect.TypeOf(S{})
		field, ok := typ.FieldByName("F")
		require.True(t, ok)
		v, err := parseVersionField(field)
		require.EqualError(t, err, "version key tag for field \"F\" is empty: \"version,\"")
		require.Nil(t, v)
	})
}
