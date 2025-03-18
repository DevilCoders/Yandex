package rule

import (
	"fmt"
	"reflect"
	"time"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

var (
	timeType = reflect.TypeOf(time.Time{})

	// fixed indexes of digits if RFC3339 time string
	rfc3339DigitParts = []int{
		0, 1, 2, 3, // year
		5, 6, // month
		8, 9, //day
		11, 12, //hour
		14, 15, // minutes
		17, 18, // seconds
	}
	// fixed indexes of timezone digits if RFC3339 time string
	rfc3339TzDigits = []int{
		0, 1, // hour
		3, 4, // minutes
	}
)

// IsRFC3339 checks if given value conforms RFC3339 format.
// Value of type time.Time is valid by default
func IsRFC3339(v *inspection.Inspected) error {
	if v.IndirectType == timeType {
		return nil
	}

	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	if len(s) < 20 {
		return ErrStringTooShort
	}

	// check constant characters
	if s[4] != '-' || s[7] != '-' || s[10] != 'T' ||
		s[13] != ':' || s[16] != ':' {
		return ErrInvalidCharsSequence
	}

	// check constant digits
	for _, idx := range rfc3339DigitParts {
		if s[idx] < '0' || s[idx] > '9' {
			return ErrInvalidCharsSequence
		}
	}

	// default index of timezone for RFC3339
	tzIdx := 19

	// fractions
	if s[tzIdx] == '.' || s[tzIdx] == ',' {
		for i := tzIdx + 1; i < len(s); i++ {
			// timezone index found
			if s[i] == 'Z' || s[i] == '-' || s[i] == '+' {
				tzIdx = i
				break
			}
			// check for non-digit char
			if s[i] < '0' || s[i] > '9' {
				return ErrInvalidCharsSequence
			}
		}
	}

	// happy path: UTC timezone
	if s[tzIdx] == 'Z' {
		return nil
	}

	// timezone after seconds/fractions
	if s[tzIdx] != '-' && s[tzIdx] != '+' {
		return ErrInvalidCharsSequence
	}

	// check timezone length
	if len(s[tzIdx+1:]) != 5 {
		return ErrInvalidCharsSequence
	}

	// check constant timezone part
	if s[tzIdx+3] != ':' {
		return ErrInvalidCharsSequence
	}

	// check constant digits
	for _, idx := range rfc3339TzDigits {
		if s[tzIdx+1+idx] < '0' || s[tzIdx+1+idx] > '9' {
			return ErrInvalidCharsSequence
		}
	}

	return nil
}
