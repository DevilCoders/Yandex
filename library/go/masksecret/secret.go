// Package masksecret provides useful functions to strip or hide
// sensitive data in various data types
package masksecret

const SecretPlaceholder = 'x'

var (
	markers = map[string]struct{}{
		"password": {},
		"passwd":   {},
		"pass":     {},
	}
)

func makePlaceholder(r rune, size int) string {
	return string(makePlaceholderRunes(r, size))
}

func makePlaceholderRunes(r rune, size int) []rune {
	p := make([]rune, size)
	for i := range p {
		p[i] = r
	}
	return p
}

func consistsOf(s string, r rune) bool {
	for _, sr := range s {
		if sr != r {
			return false
		}
	}
	return true
}
