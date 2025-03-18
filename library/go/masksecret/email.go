package masksecret

import (
	"errors"
	"strings"
)

// Email masks string secret representing email
func Email(s string) (string, error) {
	p := strings.SplitN(s, "@", 2)
	if len(p) != 2 {
		return s, errors.New("not a valid email")
	}

	return String(p[0]) + "@" + p[1], nil
}
