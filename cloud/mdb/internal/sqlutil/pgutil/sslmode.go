package pgutil

import "fmt"

// Valid ssl modes
const (
	DisableSSLMode    = "disable"
	AllowSSLMode      = "allow"
	PreferSSLMode     = "prefer"
	RequireSSLMode    = "require"
	VerifyCaSSLMode   = "verify-ca"
	VerifyFullSSLMode = "verify-full"
)

var sslModes = map[string]bool{
	DisableSSLMode:    true,
	AllowSSLMode:      true,
	PreferSSLMode:     true,
	RequireSSLMode:    true,
	VerifyCaSSLMode:   true,
	VerifyFullSSLMode: true,
}

type sslMode string

func (s sslMode) MarshalText() ([]byte, error) {
	return []byte(s.String()), nil
}

func (s *sslMode) UnmarshalText(text []byte) error {
	status := string(text)
	if ok := sslModes[status]; !ok {
		return fmt.Errorf("unknown ssl mode: %s", status)
	}

	*s = sslMode(status)
	return nil
}

func (s sslMode) String() string {
	return string(s)
}
