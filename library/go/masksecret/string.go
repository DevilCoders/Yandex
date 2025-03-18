package masksecret

import "unicode/utf8"

// String masks string secret
func String(s string) string {
	size := utf8.RuneCountInString(s)
	res := makePlaceholderRunes(SecretPlaceholder, size)

	if size < 5 {
		return string(res)
	}

	res[0], _ = utf8.DecodeRuneInString(s)
	res[size-1], _ = utf8.DecodeLastRuneInString(s)

	return string(res)
}
