package rule

import (
	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Required is a rule that checks if value must not be zero
func Required(value *inspection.Inspected) error {
	if !value.IsValid() || value.IsZero {
		return ErrRequired
	}
	return nil
}
