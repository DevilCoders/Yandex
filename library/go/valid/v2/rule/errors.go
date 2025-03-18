package rule

import (
	"errors"
	"fmt"
	"io"
	"reflect"
	"strings"
)

var (
	ErrRequired       = errors.New("required")
	ErrEmptyValue     = errors.New("empty value")
	ErrDuplicateValue = errors.New("duplicate value")

	ErrPatternMismatch = errors.New("pattern mismatch")

	ErrInvalidLength = errors.New("invalid length")

	ErrEmptyString         = errors.New("empty string given")
	ErrStringTooShort      = errors.New("given string too short")
	ErrInvalidStringLength = errors.New("invalid string length")

	ErrInvalidCharsSequence = errors.New("invalid characters sequence")
	ErrInvalidCharacters    = errors.New("invalid characters detected")
	ErrInvalidChecksum      = errors.New("invalid checksum")

	ErrOutOfRange    = errors.New("out of range")
	ErrNegativeValue = errors.New("negative value")

	ErrBadParams   = errors.New("bad validation params")
	ErrInvalidType = errors.New("invalid type")

	ErrUnexpected = errors.New("unexpected")
	ErrExpected   = errors.New("expected")
)

type Errors []error

// Error implements error type
func (es Errors) Error() string {
	return es.String()
}

// Errors implements multierr interface
func (es Errors) Errors() []error {
	return es
}

// String implements Stringer interface
func (es Errors) String() string {
	return es.join("\n")
}

// Format implements fmt.Formatter interface
func (es Errors) Format(s fmt.State, verb rune) {
	switch verb {
	case 'v':
		if s.Flag('+') {
			for i, e := range es {
				if i > 0 {
					_, _ = io.WriteString(s, "\n")
				}
				_, _ = io.WriteString(s, fmt.Sprintf("%+v", e))
			}
			return
		}
		fallthrough
	case 's', 'q':
		_, _ = io.WriteString(s, es.Error())
	}
}

// Is conforms errors.Is interface
func (es Errors) Is(err error) bool {
	for _, e := range es {
		if errors.Is(err, e) {
			return true
		}
	}
	return false
}

// As conforms errors.As interface
func (es Errors) As(target interface{}) bool {
	for _, e := range es {
		if errors.As(e, target) {
			return true
		}
	}
	return false
}

// joins errors into single string with given glue
func (es Errors) join(glue string) string {
	if len(es) == 0 {
		return ""
	}

	var b strings.Builder
	for i, e := range es {
		b.WriteString(e.Error())
		if i < len(es)-1 {
			b.WriteString(glue)
		}
	}
	return b.String()
}

// FieldError holds additional information about struct field validation error
type FieldError struct {
	// field meta
	Field *reflect.StructField
	// actual error
	err error
}

// NewFieldError returns new FieldError
func NewFieldError(field *reflect.StructField, err error) FieldError {
	return FieldError{
		Field: field,
		err:   err,
	}
}

// Error implements error type
func (e FieldError) Error() string {
	if e.err == nil {
		return ""
	}

	return e.err.Error()
}

// Format implements fmt.Formatter interface
func (e FieldError) Format(s fmt.State, verb rune) {
	switch verb {
	case 'v':
		if s.Flag('+') {
			_, _ = io.WriteString(s, e.Field.Name)

			err := e.err
			for err != nil {
				var ferr FieldError
				if errors.As(err, &ferr) {
					_, _ = io.WriteString(s, "."+ferr.Field.Name)
				}
				nerr := errors.Unwrap(err)
				if nerr == nil {
					_, _ = io.WriteString(s, ": "+err.Error())
				}
				err = nerr
			}
			return
		}
		fallthrough
	case 's', 'q':
		_, _ = io.WriteString(s, e.Error())
	}
}

// Unwrap implements errors.Unwrap interface
func (e FieldError) Unwrap() error {
	return e.err
}
