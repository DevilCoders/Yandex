package cfgtypes

import (
	"fmt"
	"strings"
)

//go:generate stringer -type=AuthType -linecomment -output enums_string.go

type AuthType uint8

func (t AuthType) MarshalText() ([]byte, error) {
	if t >= maxAuthType {
		return nil, fmt.Errorf("failed to marshal auth type: value (%d) is not in the allowed range (0-%d)", t, maxAuthType-1)
	}
	return []byte(t.String()), nil
}

func (t *AuthType) UnmarshalText(text []byte) error {
	switch val := string(text); strings.ToLower(val) {
	case NoAuthType.String():
		*t = NoAuthType
	case IAMMetaAuthType.String():
		*t = IAMMetaAuthType
	case TVMAuthType.String():
		*t = TVMAuthType
	case JWTAuthType.String():
		*t = JWTAuthType
	case TeamJWTAuthType.String():
		*t = TeamJWTAuthType
	case StaticTokenAuthType.String():
		*t = StaticTokenAuthType
	default:
		return fmt.Errorf("unknown auth type: %s", val)
	}
	return nil
}

const (
	DefaultAuthType     AuthType = iota // default
	NoAuthType                          // noauth
	IAMMetaAuthType                     // iam-metadata
	TVMAuthType                         // tvm
	JWTAuthType                         // jwt
	TeamJWTAuthType                     // team-jwt
	StaticTokenAuthType                 // static
	maxAuthType                         //_max
)
