package rule

import (
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// OmitEmpty is a wrapper that skips all given rules if value is empty.
// Emptiness checks in terms of Go, i.e. zero value of its kind.
// Result rule will always return error of type Errors
func OmitEmpty(rules ...Rule) Rule {
	return func(value *inspection.Inspected) error {
		if value.IsZero || len(rules) == 0 {
			return nil
		}

		var errs Errors
		for _, rule := range rules {
			if err := rule(value); err != nil {
				errs = append(errs, err)
			}
		}
		return errs
	}
}

// NotEmpty checks value is not empty
func NotEmpty(value *inspection.Inspected) error {
	if !value.IsValid() || value.IsZero {
		return ErrEmptyValue
	}

	k := value.Indirect.Kind()
	if (k == reflect.Slice || k == reflect.Map) &&
		value.Indirect.Len() == 0 {
		return ErrEmptyValue
	}

	return nil
}
