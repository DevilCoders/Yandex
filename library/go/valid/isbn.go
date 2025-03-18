package valid

import (
	"errors"
)

var (
	ErrInvalidISBN = errors.New("valid: given string is not a valid ISBN")
)

// ISBN check if the string is an ISBN (version 10 or 13).
func ISBN(s string) error {
	if len(s) < 10 {
		return ErrStringTooShort
	}

	isbn10Err := ISBN10(s)
	isbn13Err := ISBN13(s)

	if isbn10Err == nil || isbn13Err == nil {
		return nil
	}
	return ErrInvalidISBN
}

// ISBN10 check if the string is an ISBN version 10.
func ISBN10(s string) error {
	if len(s) < 10 {
		return ErrStringTooShort
	}

	s = stripISBN(s)
	if len(s) != 10 {
		return ErrInvalidStringLength
	}

	var sum int
	for i := 0; i < 9; i++ {
		sum += (i + 1) * int(s[i]-'0')
	}
	if s[9] == 'X' {
		sum += 10 * 10
	} else {
		sum += 10 * int(s[9]-'0')
	}

	if sum%11 != 0 {
		return ErrInvalidChecksum
	}
	return nil
}

// ISBN13 check if the string is an ISBN version 13.
func ISBN13(s string) error {
	if len(s) < 13 {
		return ErrStringTooShort
	}

	s = stripISBN(s)
	if len(s) != 13 {
		return ErrInvalidStringLength
	}

	var sum int
	for i := 0; i < 12; i++ {
		if i%2 == 0 {
			sum += int(s[i] - '0')
		} else {
			sum += int(s[i]-'0') * 3
		}
	}

	if int(s[12]-'0')-((10-(sum%10))%10) != 0 {
		return ErrInvalidChecksum
	}
	return nil
}

func stripISBN(s string) string {
	for i := len(s) - 1; i >= 0; i-- {
		if s[i] == ' ' || s[i] == '-' {
			s = s[:i] + s[i+1:]
		}
	}
	return s
}
