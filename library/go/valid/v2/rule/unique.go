package rule

import (
	"fmt"
	"reflect"

	"cuelang.org/go/pkg/strconv"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Unique checks if each value is exists only once in given set.
// Slices, maps and arrays are accepted.
// Key function param must return string representation on unique key of value
func Unique(keyFunc func(value *inspection.Inspected) string) Rule {
	store := make(map[string]struct{})
	return Each(func(value *inspection.Inspected) error {
		key := keyFunc(value)
		if _, ok := store[key]; ok {
			return fmt.Errorf("%s: %w", key, ErrDuplicateValue)
		}
		store[key] = struct{}{}
		return nil
	})
}

// ValueAsKey is a predefined key function that uses string representation of value as key
func ValueAsKey(v *inspection.Inspected) string {
	switch v.Indirect.Kind() {
	case reflect.String:
		return v.Indirect.String()
	case reflect.Int8, reflect.Int16,
		reflect.Int32, reflect.Int64, reflect.Int:
		return strconv.FormatInt(v.Indirect.Int(), 10)
	case reflect.Uint8, reflect.Uint16,
		reflect.Uint32, reflect.Uint64, reflect.Uint:
		return strconv.FormatUint(v.Indirect.Uint(), 10)
	case reflect.Float32, reflect.Float64:
		return strconv.FormatFloat(v.Indirect.Float(), 'f', 'g', 64)
	case reflect.Bool:
		return strconv.FormatBool(v.Indirect.Bool())
	default:
		return fmt.Sprint(v.Interface)
	}
}
