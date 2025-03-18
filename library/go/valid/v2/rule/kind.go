package rule

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// IsKind checks reflected value is one of given kinds
func IsKind(kinds ...reflect.Kind) Rule {
	return func(v *inspection.Inspected) error {
		k := v.Indirect.Kind()
		for _, kind := range kinds {
			if k == kind {
				return nil
			}
		}
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}
}
