package valid

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

// Struct is special function to validate struct fields.
// It will always return error as valid.Errors consisting of valid.FieldError.
func Struct(target interface{}, vrs ...ValueRule) error {
	iv := inspection.Inspect(target)
	// // must be a pointer to a struct
	if iv.Value.Kind() != reflect.Ptr || iv.Indirect.Kind() != reflect.Struct {
		return fmt.Errorf("struct pointer required")
	}
	// treat a nil struct pointer as valid
	if iv.Value.IsNil() {
		return nil
	}

	var errs rule.Errors
	for i, vr := range vrs {
		if vr.value.Value.Kind() != reflect.Ptr {
			return fmt.Errorf("pointer to field required: %d", i)
		}

		sf := findStructField(iv, vr.value)
		if sf == nil {
			return fmt.Errorf("cannot find field: %d", i)
		}

		verrs := vr.Validate()
		for _, err := range unwrapErrors(verrs) {
			errs = append(errs, rule.NewFieldError(&sf.Field, err))
		}
	}

	if len(errs) == 0 {
		return nil
	}
	return errs
}

// findStructField looks for a field in the given struct.
// The field being looked for should be a pointer to the actual struct field.
// If found, the field data will be returned, nil otherwise.
func findStructField(structValue *inspection.Inspected, fieldValue *inspection.Inspected) *inspection.StructField {
	for _, sf := range structValue.Fields {
		if sf.Value.UnsafeAddr() == fieldValue.Value.Pointer() {
			// do additional type comparison because it's possible that the address of
			// an embedded struct is the same as the first field of the embedded struct
			if sf.Field.Type == reflect.Indirect(fieldValue.Value).Type() {
				return sf
			}
		}
		if sf.Field.Anonymous {
			// delve into anonymous struct to look for the field
			fk := reflect.Indirect(sf.Value).Kind()
			if fk == reflect.Struct {
				if f := findStructField(inspection.Inspect(sf.Value), fieldValue); f != nil {
					return f
				}
			}
		}
	}
	return nil
}
