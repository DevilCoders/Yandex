package rule

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// IsUUID checks if value is a canonical UUID (version 3, 4 or 5).
func IsUUID(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()

	if len(s) != 36 {
		return ErrInvalidStringLength
	}

	if s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-' {
		return ErrInvalidCharsSequence
	}

	for _, c := range s {
		if (c < 'a' || c > 'f') && (c < '0' || c > '9') && c != '-' {
			return ErrInvalidCharacters
		}
	}

	return nil
}

// IsUUIDv3 checks if value is a canonical UUID version 3.
func IsUUIDv3(v *inspection.Inspected) error {
	if err := IsUUID(v); err != nil {
		return err
	}

	s := v.Indirect.String()
	if s[14] != '3' {
		return ErrInvalidCharsSequence
	}

	return nil
}

// IsUUIDv4 checks if value is a canonical UUID version 4.
func IsUUIDv4(v *inspection.Inspected) error {
	if err := IsUUID(v); err != nil {
		return err
	}

	s := v.Indirect.String()
	if s[14] != '4' || (s[19] != '8' && s[19] != '9' && s[19] != 'a' && s[19] != 'b') {
		return ErrInvalidCharsSequence
	}

	return nil
}

// IsUUIDv5 checks if value is a canonical UUID version 5.
func IsUUIDv5(v *inspection.Inspected) error {
	if err := IsUUID(v); err != nil {
		return err
	}

	s := v.Indirect.String()
	if s[14] != '5' || (s[19] != '8' && s[19] != '9' && s[19] != 'a' && s[19] != 'b') {
		return ErrInvalidCharsSequence
	}

	return nil
}
