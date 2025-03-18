package urlutil

import (
	"net/url"
)

// MustParse attempts to parse given URL string and panics on error
func MustParse(u string) *url.URL {
	uu, err := url.Parse(u)
	if err != nil {
		panic(err)
	}
	return uu
}
