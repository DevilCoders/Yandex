package address

import (
	"fmt"
	"net/url"
	"regexp"
)

const (
	TCP  = "tcp"
	Unix = "unix"
)

var schemaReg *regexp.Regexp

func init() {
	schemaReg = regexp.MustCompile(`^[a-z]+://`)
}

type Server struct {
	Network string
	Address string
}

// Parse server address extracting network and address
func Parse(addr string) (Server, error) {
	/*
		In most use-cases address includes only port:

		   1. ':5000'
		   2. '[::]:5000'
		   3. 'localhost:5000'

		url.Parse can't handle such addresses:
		   1. parse ":50": missing protocol scheme
		   2. parse "[::]:50": first path segment in URL cannot contain colon
		   3. localhost:5000 -> {"Scheme":"localhost", "Opaque":"50", "Host":"", "Path":"" ...

		That is reasonable, because that samples are not valid URLs.
	*/
	if !schemaReg.MatchString(addr) {
		return Server{Network: TCP, Address: addr}, nil
	}
	u, err := url.Parse(addr)
	if err != nil {
		return Server{}, fmt.Errorf("parse url: %w", err)
	}
	switch u.Scheme {
	case Unix:
		return Server{Network: Unix, Address: u.Path}, nil
	case TCP:
		return Server{Network: TCP, Address: u.Host}, nil
	default:
		return Server{}, fmt.Errorf("unsupported address scheme '%s' expecting %s or %s: %s", u.Scheme, TCP, Unix, addr)
	}
}
