package nacl

import (
	"encoding/base64"
	"fmt"
)

const keyLength = 32

// ParseKey parses key encoded in base64 string
func ParseKey(keyStr string) ([keyLength]byte, error) {
	var key [keyLength]byte
	ba, err := base64.URLEncoding.DecodeString(keyStr)
	if err != nil {
		return key, fmt.Errorf("failed to load key: %s", err.Error())
	}
	if len(ba) != keyLength {
		return key, fmt.Errorf("key should be %d bytes long, but it is %d bytes", len(key), len(ba))
	}
	copy(key[:keyLength], ba[:])
	return key, nil
}

// EncodeKey encodes key to string
func EncodeKey(key [keyLength]byte) string {
	return base64.URLEncoding.EncodeToString(key[:])
}
