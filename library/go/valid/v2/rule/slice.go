package rule

import (
	"errors"
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// InSlice checks if value or pointer to value is present in given slice.
// Slice and value types must be equal.
func InSlice(values interface{}) Rule {
	slice := reflect.ValueOf(values)
	return func(v *inspection.Inspected) error {
		if slice.Kind() != reflect.Slice {
			return fmt.Errorf("slice of %s expected: %w", v.TypeName, ErrInvalidType)
		}
		indirectIface := v.Indirect.Interface()
		for i := 0; i < slice.Len(); i++ {
			svi := slice.Index(i).Interface()
			if svi == v.Interface || svi == indirectIface {
				return nil
			}
		}
		return ErrUnexpected
	}
}

// NotInSlice checks if value or pointer to value is not present in given slice.
// Slice and value types must be equal.
func NotInSlice(values interface{}) Rule {
	r := InSlice(values)
	return func(v *inspection.Inspected) error {
		err := r(v)
		if errors.Is(err, ErrUnexpected) {
			return nil
		}
		if err == nil {
			return ErrExpected
		}
		return err
	}
}
