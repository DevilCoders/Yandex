package rule

import (
	"fmt"
	"math"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// InRange checks value is in given range
func InRange(min, max float64) Rule {
	err := fmt.Errorf("value must be between %g and %g: %w", min, max, ErrOutOfRange)
	return func(v *inspection.Inspected) error {
		switch v.Indirect.Kind() {
		case reflect.Int8, reflect.Int16,
			reflect.Int32, reflect.Int64, reflect.Int:
			fval := float64(v.Indirect.Int())
			if fval < min || fval > max {
				return err
			}
		case reflect.Uint8, reflect.Uint16,
			reflect.Uint32, reflect.Uint64, reflect.Uint:
			fval := float64(v.Indirect.Uint())
			if fval < min || fval > max {
				return err
			}
		case reflect.Float32, reflect.Float64:
			fval := v.Indirect.Float()
			if fval < min || fval > max {
				return err
			}
		default:
			return fmt.Errorf("%s: %w", v.Indirect.Kind(), ErrInvalidType)
		}

		return nil
	}
}

// LesserOrEqual checks value is lesser or equal to given
func LesserOrEqual(lim float64) Rule {
	return InRange(math.Inf(-1), lim)
}

// GreaterOrEqual checks value is greater or equal to given
func GreaterOrEqual(lim float64) Rule {
	return InRange(lim, math.Inf(1))
}

// IsPositive checks if value is positive.
// It checks ints, uints, floats and booleans
func IsPositive(v *inspection.Inspected) error {
	switch v.Indirect.Kind() {
	case reflect.Int8, reflect.Int16,
		reflect.Int32, reflect.Int64, reflect.Int:
		if v.Indirect.Int() < 0 {
			return ErrNegativeValue
		}
	case reflect.Uint8, reflect.Uint16,
		reflect.Uint32, reflect.Uint64, reflect.Uint:
		return nil
	case reflect.Float32, reflect.Float64:
		if v.Indirect.Float() < 0 {
			return ErrNegativeValue
		}
	case reflect.Bool:
		if !v.Indirect.Bool() {
			return ErrNegativeValue
		}
	default:
		return fmt.Errorf("%s: %w", v.Indirect.Kind(), ErrInvalidType)
	}

	return nil
}
