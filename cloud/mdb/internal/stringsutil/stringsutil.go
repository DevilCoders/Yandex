package stringsutil

import "strings"

// QuotedJoin is strings.Join with quoted values
func QuotedJoin(a []string, sep, quote string) string {
	switch len(a) {
	case 0:
		return ""
	case 1:
		return quote + a[0] + quote
	}
	n := len(sep) * (len(a) - 1)
	for i := 0; i < len(a); i++ {
		n += len(a[i])
	}

	var b strings.Builder
	b.Grow(n)
	b.WriteString(quote)
	b.WriteString(a[0])
	b.WriteString(quote)
	for _, s := range a[1:] {
		b.WriteString(sep)
		b.WriteString(quote)
		b.WriteString(s)
		b.WriteString(quote)
	}

	return b.String()
}

// MapJoinStr is strings.Join for map keys
func MapJoinStr(a map[string]string, sep string) string {
	switch len(a) {
	case 0:
		return ""
	case 1:
		for k := range a {
			return k
		}
	}

	n := len(sep) * (len(a) - 1)
	for k := range a {
		n += len(k)
	}

	var b strings.Builder
	b.Grow(n)

	var firstDone bool
	for k := range a {
		if firstDone {
			b.WriteString(sep)
		}

		b.WriteString(k)
		firstDone = true
	}

	return b.String()
}

// MapJoin is strings.Join for map keys
func MapJoin(a map[string]struct{}, sep string) string {
	switch len(a) {
	case 0:
		return ""
	case 1:
		for k := range a {
			return k
		}
	}

	n := len(sep) * (len(a) - 1)
	for k := range a {
		n += len(k)
	}

	var b strings.Builder
	b.Grow(n)

	var firstDone bool
	for k := range a {
		if firstDone {
			b.WriteString(sep)
		}

		b.WriteString(k)
		firstDone = true
	}

	return b.String()
}

// MapQuotedJoin is MapJoin with quoted values
func MapQuotedJoin(a map[string]struct{}, sep, quote string) string {
	switch len(a) {
	case 0:
		return ""
	case 1:
		for k := range a {
			return quote + k + quote
		}
	}

	n := len(sep)*(len(a)-1) + 2*len(quote)*len(a)
	for k := range a {
		n += len(k)
	}

	var b strings.Builder
	b.Grow(n)

	var firstDone bool
	for k := range a {
		if firstDone {
			b.WriteString(sep)
		}

		b.WriteString(quote)
		b.WriteString(k)
		b.WriteString(quote)
		firstDone = true
	}

	return b.String()
}
