package rule

import (
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Each returns new ValueRule that loops through an iterable (map, slice or array)
// and validates each value inside with the provided rules.
// Rule will return nil error if non-iterable value given
func Each(rules ...Rule) Rule {
	return func(value *inspection.Inspected) error {
		var errs Errors

		switch value.Indirect.Kind() {
		case reflect.Map:
			iter := value.Indirect.MapRange()
			for iter.Next() {
				iv := inspection.Inspect(iter.Value())
				// call Validator interface
				if iv.Validate != nil {
					if verrs := iv.Validate(); verrs != nil {
						errs = append(errs, verrs)
					}
				}
				// call rules
				for _, r := range rules {
					if verrs := r(iv); verrs != nil {
						errs = append(errs, verrs)
					}
				}
			}
		case reflect.Slice, reflect.Array:
			for i := 0; i < value.Indirect.Len(); i++ {
				iv := inspection.Inspect(value.Indirect.Index(i))
				// call Validator interface
				if iv.Validate != nil {
					if verrs := iv.Validate(); verrs != nil {
						errs = append(errs, verrs)
					}
				}
				// call rules
				for _, r := range rules {
					if verrs := r(iv); verrs != nil {
						errs = append(errs, verrs)
					}
				}
			}
		default:
			return nil
		}

		if len(errs) == 0 {
			return nil
		}
		return errs
	}
}
