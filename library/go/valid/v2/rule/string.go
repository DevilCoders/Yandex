package rule

import (
	"fmt"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// IsAlphanumeric check if value contains only letters and numbers
func IsAlphanumeric(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if len(s) == 0 {
		return ErrEmptyString
	}

	for i := 0; i < len(s); i++ {
		if ('Z' < s[i] || s[i] < 'A') && ('z' < s[i] || s[i] < 'a') && ('9' < s[i] || s[i] < '0') {
			return ErrInvalidCharacters
		}
	}
	return nil
}

// IsAlpha checks if value contains only ASCII letters
func IsAlpha(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if len(s) == 0 {
		return ErrEmptyString
	}

	for i := 0; i < len(s); i++ {
		if ('Z' < s[i] || s[i] < 'A') && ('z' < s[i] || s[i] < 'a') {
			return ErrInvalidCharacters
		}
	}
	return nil
}

// IsASCII check if string value contains only printable ASCII symbols.
func IsASCII(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if len(s) == 0 {
		return ErrEmptyString
	}

	const space, tilde = 0x20, 0x7e
	for i := 0; i < len(s); i++ {
		if s[i] < space || s[i] > tilde {
			return ErrInvalidCharacters
		}
	}
	return nil
}

// IsHexColor check if string value represents hexadecimal color
func IsHexColor(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if len(s) < 3 {
		return ErrStringTooShort
	}

	if s[0] == '#' {
		s = s[1:]
	}

	if len(s) != 3 && len(s) != 6 {
		return ErrInvalidStringLength
	}

	for i := 0; i < len(s); i++ {
		if ('F' < s[i] || s[i] < 'A') && ('f' < s[i] || s[i] < 'a') && ('9' < s[i] || s[i] < '0') {
			return ErrInvalidCharacters
		}
	}

	return nil
}

// HasPrefix checks if string value has given prefix
func HasPrefix(prefix string) Rule {
	return func(v *inspection.Inspected) error {
		if k := v.Indirect.Kind(); k != reflect.String {
			return fmt.Errorf("%s: %w", k, ErrInvalidType)
		}
		if !strings.HasPrefix(v.Indirect.String(), prefix) {
			return ErrPatternMismatch
		}
		return nil
	}
}

// HasSuffix checks if string value has given suffix
func HasSuffix(suffix string) Rule {
	return func(v *inspection.Inspected) error {
		if k := v.Indirect.Kind(); k != reflect.String {
			return fmt.Errorf("%s: %w", k, ErrInvalidType)
		}
		if !strings.HasSuffix(v.Indirect.String(), suffix) {
			return ErrPatternMismatch
		}
		return nil
	}
}

// Is2DMeasurements checks if string value represents 2D measurements delimited by given separator
// (e.g. 200x150 as image width and height)
func Is2DMeasurements(separator string) Rule {
	return func(v *inspection.Inspected) error {
		if k := v.Indirect.Kind(); k != reflect.String {
			return fmt.Errorf("%s: %w", k, ErrInvalidType)
		}

		s := v.Indirect.String()
		idx := strings.Index(s, separator)
		if idx == -1 {
			return ErrPatternMismatch
		}
		if err := numeric(s[:idx]); err != nil {
			return ErrPatternMismatch
		}
		if err := numeric(s[idx+1:]); err != nil {
			return ErrPatternMismatch
		}
		return nil
	}
}

// IsNumeric check if string value contains only numbers.
func IsNumeric(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}
	return numeric(v.Indirect.String())
}

// numeric checks if string contains only numbers.
func numeric(s string) error {
	if len(s) == 0 {
		return ErrEmptyString
	}

	for i := 0; i < len(s); i++ {
		if '9' < s[i] || s[i] < '0' {
			return ErrInvalidCharacters
		}
	}

	return nil
}
