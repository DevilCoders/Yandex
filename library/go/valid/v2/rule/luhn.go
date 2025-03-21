package rule

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Luhn is an implementation of Luhn algorithm.
// For more information see https://en.wikipedia.org/wiki/Luhn_algorithm
func Luhn(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if len(s) == 0 {
		return ErrEmptyString
	}

	var sum int
	var alter bool
	for i := len(s) - 1; i >= 0; i-- {
		d := int(s[i] - '0')
		if alter {
			d *= 2
		}
		sum += d / 10
		sum += d % 10
		alter = !alter
	}

	if sum%10 != 0 {
		return ErrInvalidChecksum
	}
	return nil
}
