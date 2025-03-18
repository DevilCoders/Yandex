package rule

import (
	"fmt"
	"reflect"

	"a.yandex-team.ru/library/go/valid/v2/inspection"
)

// Semver check if string value is valid semantic version.
// Check based on https://semver.org spec
func Semver(v *inspection.Inspected) error {
	if k := v.Indirect.Kind(); k != reflect.String {
		return fmt.Errorf("%s: %w", k, ErrInvalidType)
	}

	s := v.Indirect.String()
	// minimal valid string is "0.0.1"
	if len(s) < 5 {
		return ErrStringTooShort
	}

	// strip leading "v"
	if s[0] == 'v' {
		s = s[1:]
	}

	majorEnd, minorEnd, patchEnd := -1, -1, -1
	preStart, metaStart := -1, -1
	for i := 0; i < len(s); i++ {
		// detect major/minor parts first
		if majorEnd == -1 || minorEnd == -1 {
			if s[i] == '.' && majorEnd == -1 {
				majorEnd = i
				continue
			}
			if s[i] == '.' && minorEnd == -1 {
				minorEnd = i
				continue
			}
			// runes in major/minor must be numeric
			if s[i] < '0' || s[i] > '9' {
				return ErrInvalidCharsSequence
			}
		}

		// detect patch part
		if majorEnd != -1 || minorEnd != -1 {
			// save patch part end position
			if patchEnd == -1 && (s[i] == '-' || s[i] == '+') {
				patchEnd = i
			}
			// save meta position
			if metaStart == -1 && s[i] == '+' {
				metaStart = i
				continue
			}
			// save pre-release position
			if preStart == -1 && metaStart == -1 && s[i] == '-' {
				preStart = i
				continue
			}
			// runes in patch must be numeric
			if patchEnd == -1 && (s[i] < '0' || s[i] > '9') {
				return ErrInvalidCharsSequence
			}
			// runes in pre-release must be ASCII
			if preStart != -1 {
				if ('Z' < s[i] || s[i] < 'A') &&
					('z' < s[i] || s[i] < 'a') &&
					('9' < s[i] || s[i] < '0') &&
					s[i] != '.' && s[i] != '-' {
					return ErrInvalidCharacters
				}
				// check for leading zeroes in pre-release
				if (s[i-2] == '.' || s[i-2] == '-') &&
					(s[i] >= '0' && s[i] <= '9') &&
					s[i-1] == '0' {
					return ErrInvalidCharsSequence
				}
			}
			// runes in meta must be ASCII
			if metaStart != -1 {
				if ('Z' < s[i] || s[i] < 'A') &&
					('z' < s[i] || s[i] < 'a') &&
					('9' < s[i] || s[i] < '0') &&
					s[i] != '-' {
					return ErrInvalidCharacters
				}
			}
		}
	}

	// patch ends on string end if no pre-release or meta found
	if patchEnd == -1 {
		patchEnd = len(s)
	}

	if majorEnd == -1 || minorEnd == -1 {
		return ErrInvalidCharsSequence
	}

	// check parts positions
	if patchEnd <= minorEnd || minorEnd <= majorEnd {
		return ErrInvalidCharsSequence
	}
	if metaStart != -1 && metaStart <= preStart {
		return ErrInvalidCharsSequence
	}

	// check that major/minor/patch parts does not have leading zero
	if (majorEnd > 1 && s[0] == '0') ||
		(minorEnd-majorEnd > 2 && s[majorEnd+1] == '0') ||
		(patchEnd-minorEnd > 2 && s[minorEnd+1] == '0') {
		return ErrInvalidCharsSequence
	}

	return nil
}
