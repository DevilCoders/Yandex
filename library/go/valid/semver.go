package valid

import (
	"errors"
	"strings"
)

var (
	ErrTooFewSemverParts  = errors.New("valid: too few semver parts")
	ErrVersionLeadingZero = errors.New("valid: leading zero prohibited in version part")
	ErrNonNumericVersion  = errors.New("valid: version contains non-numeric characters")
)

// Semver check if string is valid semantic version
func Semver(s string) error {
	// minimal valid string is "0.0.1"
	if len(s) < 5 {
		return ErrStringTooShort
	}

	// invalid special chars sequence
	if strings.Contains(s, "+-") || strings.Contains(s, "-+") {
		return ErrInvalidCharsSequence
	}

	// strip leading "v"
	if s[0] == 'v' {
		s = s[1:]
	}

	// get clean major, clean minor and maybe dirty patch parts
	p := strings.SplitN(s, ".", 3)
	if len(p) != 3 {
		return ErrTooFewSemverParts
	}

	// try to extract meta part from patch first
	p = append(p[:2], strings.SplitN(p[2], "+", 2)...)
	if len(p) == 3 {
		// no meta - try to extract pre-release
		p = append(p[:2], strings.SplitN(p[2], "-", 2)...)
	} else {
		// try to extract pre-release and move meta to the end
		pr := strings.SplitN(p[2], "-", 2)
		if len(pr) == 2 {
			p = append(p[:2], pr[0], pr[1], p[3])
		}
	}

	// check that major does not have leading zero
	if len(p[0]) > 1 && p[0][0] == '0' {
		return ErrVersionLeadingZero
	}

	// check that minor does not have leading zero
	if len(p[1]) > 1 && p[1][0] == '0' {
		return ErrVersionLeadingZero
	}

	// check that patch does not have leading zero
	if len(p[2]) > 1 && p[2][0] == '0' {
		return ErrVersionLeadingZero
	}

	// check that major, minor and patch are numeric
	if Numeric(p[0]) != nil || Numeric(p[1]) != nil || Numeric(p[2]) != nil {
		return ErrNonNumericVersion
	}

	// if pre-release or meta exists - check them
	if len(p) > 3 {
		for i, sp := range p[3:] {
			// neither pre-release nor meta parts must not have characters except [0-9a-zA-Z-]
			for _, v := range sp {
				if ('Z' < v || v < 'A') && ('z' < v || v < 'a') && ('9' < v || v < '0') && v != '.' && v != '-' {
					return ErrInvalidCharacters
				}
			}

			// pre-release part must not have leading zeroes in numeric parts
			if i == 0 {
				for _, psp := range strings.Split(sp, ".") {
					if len(psp) > 1 && psp[0] == '0' && (psp[1] >= '0' && psp[1] <= '9') {
						return ErrVersionLeadingZero
					}
				}
			}
		}
	}

	return nil
}
