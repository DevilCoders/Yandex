package valid

import (
	"errors"
	"reflect"
	"strconv"
	"time"

	"github.com/spf13/cast"
)

var (
	ErrLesserValue           = errors.New("valid: value less than expected")
	ErrGreaterValue          = errors.New("valid: value greater than expected")
	ErrEqual                 = errors.New("valid: value and param are equal")
	ErrNotEqual              = errors.New("valid: value and param are not equal")
	ErrUnknownComparator     = errors.New("valid: unknown comparator")
	ErrUnsupportedComparator = errors.New("valid: unsupported comparator")
)

type comparator int

const (
	compareEq comparator = iota
	compareNeq
	compareLt
	compareLte
	compareGt
	compareGte
)

// Equal checks if given value equals to param.
// These function have very complex behavior.
// Do not use it outside of validation context.
func Equal(value reflect.Value, param string) error {
	r, err := compareValues(value, param, compareEq)
	if err != nil {
		return err
	}
	if !r {
		return ErrNotEqual
	}
	return nil
}

// NotEqual checks if given value doesn't equal to param.
// These function have very complex behavior.
// Do not use it outside of validation context.
func NotEqual(value reflect.Value, param string) error {
	r, err := compareValues(value, param, compareNeq)
	if err != nil {
		return err
	}
	if !r {
		return ErrEqual
	}
	return nil
}

// Min checks if given value greater or equal to minimum param.
// These function have very complex behavior.
// Do not use it outside of validation context.
func Min(value reflect.Value, min string) error {
	r, err := compareValues(value, min, compareGte)
	if err != nil {
		return err
	}
	if !r {
		return ErrLesserValue
	}
	return nil
}

// Max checks if given value lesser or equal to maximum param.
// These function have very complex behavior.
// Do not use it outside of validation context.
func Max(value reflect.Value, max string) error {
	r, err := compareValues(value, max, compareLte)
	if err != nil {
		return err
	}
	if !r {
		return ErrGreaterValue
	}
	return nil
}

// Lesser checks if given value lesser than minimum param.
// These function have very complex behavior.
// Do not use it outside of validation context.
func Lesser(value reflect.Value, min string) error {
	r, err := compareValues(value, min, compareLt)
	if err != nil {
		return err
	}
	if !r {
		return ErrGreaterValue
	}
	return nil
}

// Greater checks if given value greater than minimum param.
// These function have very complex behavior.
// Do not use it outside of validation context.
func Greater(value reflect.Value, min string) error {
	r, err := compareValues(value, min, compareGt)
	if err != nil {
		return err
	}
	if !r {
		return ErrLesserValue
	}
	return nil
}

func compareValues(value reflect.Value, param string, cmp comparator) (bool, error) {
	switch value.Kind() {
	case reflect.Ptr:
		// unref pointer
		return compareValues(value.Elem(), param, cmp)
	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		pv, err := strconv.ParseInt(param, 10, 0)
		if err != nil {
			return false, ErrInvalidType.Wrap(err)
		}
		v := value.Int()
		switch cmp {
		case compareEq:
			return v == pv, nil
		case compareNeq:
			return v != pv, nil
		case compareLt:
			return v < pv, nil
		case compareLte:
			return v <= pv, nil
		case compareGt:
			return v > pv, nil
		case compareGte:
			return v >= pv, nil
		default:
			return false, ErrUnknownComparator
		}

	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
		pv, err := strconv.ParseUint(param, 10, 0)
		if err != nil {
			return false, ErrInvalidType.Wrap(err)
		}
		v := value.Uint()
		switch cmp {
		case compareEq:
			return v == pv, nil
		case compareNeq:
			return v != pv, nil
		case compareLt:
			return v < pv, nil
		case compareLte:
			return v <= pv, nil
		case compareGt:
			return v > pv, nil
		case compareGte:
			return v >= pv, nil
		default:
			return false, ErrUnknownComparator
		}

	case reflect.Float32, reflect.Float64:
		pv, err := strconv.ParseFloat(param, 64)
		if err != nil {
			return false, ErrInvalidType.Wrap(err)
		}
		v := value.Float()
		switch cmp {
		case compareEq:
			return v == pv, nil
		case compareNeq:
			return v != pv, nil
		case compareLt:
			return v < pv, nil
		case compareLte:
			return v <= pv, nil
		case compareGt:
			return v > pv, nil
		case compareGte:
			return v >= pv, nil
		default:
			return false, ErrUnknownComparator
		}

	case reflect.String:
		v := value.String()
		switch cmp {
		case compareEq:
			return v == param, nil
		case compareNeq:
			return v != param, nil
		case compareLt, compareLte, compareGt, compareGte:
			return false, ErrUnsupportedComparator
		default:
			return false, ErrUnknownComparator
		}

	case reflect.Bool:
		pv, err := strconv.ParseBool(param)
		if err != nil {
			return false, ErrInvalidType.Wrap(err)
		}
		v := value.Bool()
		switch cmp {
		case compareEq:
			return v == pv, nil
		case compareNeq:
			return v != pv, nil
		case compareLt, compareLte, compareGt, compareGte:
			return false, ErrUnsupportedComparator
		default:
			return false, ErrUnknownComparator
		}

	case reflect.Slice, reflect.Map:
		pv, err := strconv.ParseUint(param, 10, 0)
		if err != nil {
			return false, ErrInvalidType.Wrap(err)
		}
		l := value.Len()
		switch cmp {
		case compareEq:
			return l == int(pv), nil
		case compareNeq:
			return l != int(pv), nil
		case compareLt:
			return l < int(pv), nil
		case compareLte:
			return l <= int(pv), nil
		case compareGt:
			return l > int(pv), nil
		case compareGte:
			return l >= int(pv), nil
		default:
			return false, ErrUnknownComparator
		}
	}

	// reflect types without predefined kind
	switch v := value.Interface().(type) {
	case time.Time:
		pv, err := cast.ToTimeE(param)
		if err != nil {
			return false, ErrInvalidType.Wrap(err)
		}
		switch cmp {
		case compareEq:
			return v.Equal(pv), nil
		case compareNeq:
			return !v.Equal(pv), nil
		case compareLt:
			return v.Before(pv), nil
		case compareLte:
			return v.Equal(pv) || v.Before(pv), nil
		case compareGt:
			return v.After(pv), nil
		case compareGte:
			return v.Equal(pv) || v.After(pv), nil
		default:
			return false, ErrUnknownComparator
		}
	}

	return false, ErrInvalidType
}
