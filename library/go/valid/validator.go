package valid

import "reflect"

// Validator declares basic validation interface.
// First return param controls recursive validation flow.
// If true returned, Struct method will continue validation of current branch even if error has been returned.
type Validator interface {
	Validate(*ValidationCtx) (bool, error)
}

type ValidatorFunc func(value reflect.Value, param string) error

// WrapValidator converts any predefined validation func to ValidatorFunc.
// Example:
//     ctx := valid.NewValidationCtx()
//     ctx.Add("uuid4", valid.WrapValidator(valid.UUIDv4))
func WrapValidator(f interface{}) ValidatorFunc {
	switch ft := f.(type) {
	// basic string validator
	case func(string) error:
		return func(value reflect.Value, _ string) error {
			if value.Kind() == reflect.String {
				return ft(value.String())
			}
			return ErrInvalidType
		}
	}

	panic("valid: cannot convert to supported ValidatorFunc")
}
