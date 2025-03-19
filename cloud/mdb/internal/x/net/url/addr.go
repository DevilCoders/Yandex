package url

import (
	"fmt"
	"net/url"
)

func AddrFromURL(u *url.URL) string {
	port := u.Port()
	if port == "" {
		switch u.Scheme {
		case "http":
			port = "80"
		case "https":
			port = "443"
		}
	}

	return fmt.Sprintf("%s:%s", u.Hostname(), port)
}
