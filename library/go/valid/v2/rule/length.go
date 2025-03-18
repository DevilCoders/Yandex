package rule

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Len checks if length of value len is in given ranges.
// Supported types are: slice, map, array, string
func Len(min, max int) Rule {
	return func(v *inspection.Inspected) error {
		if max > 0 && max < min {
			return ErrBadParams
		}

		if k := v.Indirect.Kind(); k != reflect.Slice &&
			k != reflect.Map &&
			k != reflect.Array &&
			k != reflect.String {
			return fmt.Errorf("%s: %w", k, ErrInvalidType)
		}

		vlen := v.Indirect.Len()
		if max < 0 { // set no upper bound if max len is negative
			max = vlen
		}

		if vlen < min || vlen > max {
			return ErrInvalidLength
		}
		return nil
	}
}

// MinLen checks if length of value len is greater or equal to given one.
// Supported types are: slice, map, array, string
func MinLen(min int) Rule {
	return Len(min, -1)
}

// MaxLen checks if length of value len is less or equal to given one.
// Supported types are: slice, map, array, string
func MaxLen(max int) Rule {
	return Len(0, max)
}
