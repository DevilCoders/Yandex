package valid

import (
	"errors"
	"strings"
)

const (
	dataURLPrefix    = "data:"
	base64DataPrefix = "base64,"
)

var (
	ErrTooFewDataParts = errors.New("too few data parts")
	ErrEmptyDataPart   = errors.New("empty data part")
)

// DataURL checks if string is a valid RFC 2397 string.
func DataURL(s string) error {
	if s == "" {
		return ErrEmptyString
	}

	// minimum valid data URI is 'data:,'
	if len(s) < 6 {
		return ErrStringTooShort
	}

	// special case for short declaration
	if s == dataURLPrefix+"," {
		return nil
	}

	// string must start with 'data:' prefix
	if s[:5] != dataURLPrefix {
		return ErrInvalidPrefix
	}

	stringParts := strings.Split(s[5:], ";")

	// string must have at least one ; separated part
	if len(stringParts) < 2 {
		return ErrTooFewDataParts
	}

	// last part must separate data from type with comma
	if !strings.Contains(stringParts[len(stringParts)-1], ",") {
		return ErrInvalidCharsSequence
	}

	return nil
}

// Base64DataURL checks if string is a valid RFC 2397 Base64 encoded string.
func Base64DataURL(s string) error {
	if s == "" {
		return ErrEmptyString
	}

	// minimum valid base64 data URI is 'data:;base64,'
	if len(s) < 13 {
		return ErrStringTooShort
	}

	// special case for short declaration
	if s == dataURLPrefix+";"+base64DataPrefix {
		return nil
	}

	// string must start with 'data:' prefix
	if s[:5] != dataURLPrefix {
		return ErrInvalidPrefix
	}

	stringParts := strings.Split(s[5:], ";")

	// string must have at least one ; separated part
	if len(stringParts) < 2 {
		return ErrTooFewDataParts
	}

	lastPart := stringParts[len(stringParts)-1]

	// last part must separate data drom type with type and comma
	if len(lastPart) < 7 || lastPart[:7] != base64DataPrefix {
		return ErrInvalidCharsSequence
	}

	// check base64 characters range
	for _, c := range lastPart[7:] {
		if (c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') &&
			c != '+' && c != '/' && c != '=' {
			return ErrInvalidCharacters
		}
	}

	return nil
}
