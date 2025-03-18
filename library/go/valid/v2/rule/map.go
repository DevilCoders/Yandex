package rule

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// HasKey checks if map value has any of given keys
func HasKey(keys ...interface{}) Rule {
	rk := make([]reflect.Value, len(keys))
	for i, k := range keys {
		rk[i] = reflect.ValueOf(k)
	}
	return func(v *inspection.Inspected) error {
		if k := v.Indirect.Kind(); k != reflect.Map {
			return fmt.Errorf("%v: %w", k, ErrInvalidType)
		}

		for _, k := range rk {
			val := v.Indirect.MapIndex(k)
			if val.IsValid() {
				return nil
			}
		}

		return ErrUnexpected
	}
}
