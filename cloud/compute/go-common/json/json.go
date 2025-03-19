package json

import (
	"bytes"
	"encoding/json"
	"fmt"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/valid"
)

var (
	vctx = valid.NewValidationCtx()
)

type ValidationError struct {
	error string
}

func newValidationError(format string, args ...interface{}) *ValidationError {
	return &ValidationError{fmt.Sprintf(format, args...)}
}

func (e *ValidationError) Error() string {
	return e.error
}

type ParseParams struct {
	IgnoreUnknown bool
}

func init() {
	vctx.Add("notnull", func(value reflect.Value, _ string) error {
		if value.IsNil() {
			return fmt.Errorf("the field should not be null")
		}
		return nil
	})
	vctx.Add("required", func(value reflect.Value, _ string) error {
		if isZero(value) {
			return fmt.Errorf("value is required")
		}
		return nil
	})
}

func Parse(data []byte, result interface{}, params ParseParams) error {
	decoder := json.NewDecoder(bytes.NewReader(data))
	if !params.IgnoreUnknown {
		decoder.DisallowUnknownFields()
	}
	if err := decoder.Decode(result); err != nil {
		errorString := err.Error()
		unknownFieldErrorPrefix := "unknown field "

		if pos := strings.Index(errorString, unknownFieldErrorPrefix); pos >= 0 {
			return newValidationError("Invalid parameter: %s", errorString[pos+len(unknownFieldErrorPrefix):])
		}
		return err
	}

	if verr := valid.Struct(vctx, result); verr != nil {
		err := verr.(valid.Errors)[0]
		if validationErr, ok := err.(valid.FieldError); ok {
			return newValidationError("field %v; err %v", validationErr.Field(), validationErr.Error())
		}
		return err
	}

	return nil
}

func isZero(v reflect.Value) bool {
	switch v.Kind() {
	case reflect.Array, reflect.Map, reflect.Slice, reflect.String:
		return v.Len() == 0
	case reflect.Bool:
		return !v.Bool()
	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		return v.Int() == 0
	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64, reflect.Uintptr:
		return v.Uint() == 0
	case reflect.Float32, reflect.Float64:
		return v.Float() == 0
	case reflect.Interface, reflect.Ptr:
		return v.IsNil()
	}
	return false
}
