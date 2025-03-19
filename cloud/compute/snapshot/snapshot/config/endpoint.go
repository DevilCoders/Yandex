package config

import (
	"bytes"
	"fmt"
)

// ServeEndpoint describes an endpoint to serve at.
type ServeEndpoint struct {
	Network string
	Addr    string
}

func (e *ServeEndpoint) String() string {
	return fmt.Sprintf("%s:%s", e.Network, e.Addr)
}

// UnmarshalText parses from a text buffer.
func (e *ServeEndpoint) UnmarshalText(text []byte) error {
	pos := bytes.Index(text, []byte("://"))
	if pos == -1 {
		return fmt.Errorf("invalid endpoint %s. Must be in a form porto://addr", text)
	}

	e.Network, e.Addr = string(text[:pos]), string(text[pos+3:])
	switch e.Network {
	case "":
		return fmt.Errorf("empty network")
	case "tcp", "tcp6", "tcp4":
	case "unix":
	default:
		return fmt.Errorf("invalid network %s", e.Network)
	}

	if e.Addr == "" {
		return fmt.Errorf("empty address")
	}

	return nil
}
