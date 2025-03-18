package valid

import (
	"strings"
	"unicode/utf8"
)

// Alphanumeric check if the string contains only letters and numbers.
func Alphanumeric(s string) error {
	if len(s) == 0 {
		return ErrEmptyString
	}

	for _, v := range s {
		if ('Z' < v || v < 'A') && ('z' < v || v < 'a') && ('9' < v || v < '0') {
			return ErrInvalidCharacters
		}
	}

	return nil
}

// Alpha check if the string contains only letters (a-zA-Z).
func Alpha(s string) error {
	if len(s) == 0 {
		return ErrEmptyString
	}

	for _, v := range s {
		if ('Z' < v || v < 'A') && ('z' < v || v < 'a') {
			return ErrInvalidCharacters
		}
	}
	return nil
}

// Numeric check if the string contains only numbers.
func Numeric(s string) error {
	if len(s) == 0 {
		return ErrEmptyString
	}

	for _, v := range s {
		if '9' < v || v < '0' {
			return ErrInvalidCharacters
		}
	}

	return nil
}

// Double check if string is a valid double number.
func Double(s string) error {
	if len(s) == 0 {
		return ErrEmptyString
	}

	for _, v := range s {
		if ('9' < v || v < '0') && v != '.' {
			return ErrInvalidCharacters
		}
	}

	if len(s) > 1 && s[0] == '0' && s[1] != '.' {
		return ErrBadFormat
	}

	if strings.HasPrefix(s, ".") ||
		strings.HasSuffix(s, ".") {
		return ErrBadFormat
	}

	return nil
}

// HexColor check if the string is a hexadecimal color.
func HexColor(s string) error {
	if len(s) < 3 {
		return ErrStringTooShort
	}

	if s[0] == '#' {
		s = s[1:]
	}

	if len(s) != 3 && len(s) != 6 {
		return ErrInvalidStringLength
	}

	for _, c := range s {
		if ('F' < c || c < 'A') && ('f' < c || c < 'a') && ('9' < c || c < '0') {
			return ErrInvalidCharacters
		}
	}

	return nil
}

// StringLenConstraints checks for constraints of StringLen function
func StringLenConstraints(min, max int) error {
	if max < min {
		return ErrBadParams
	}

	return nil
}

// StringLen check string's length (including multi byte strings)
func StringLen(s string, min, max int) error {
	slen := utf8.RuneCountInString(s)

	if min > 0 && max > 0 {
		if err := StringLenConstraints(min, max); err != nil {
			return err
		}

		if slen < min {
			return ErrStringTooShort
		}
		if slen > max {
			return ErrStringTooLong
		}
		return nil
	}

	if min > 0 {
		if slen < min {
			return ErrStringTooShort
		}
		return nil
	}
	if max > 0 {
		if slen > max {
			return ErrStringTooLong
		}
		return nil
	}

	return nil
}
