package uuid

import (
	"crypto/rand"
	"encoding/base32"
	"fmt"
	"strings"
)

type CloudIDGenerator interface {
	GenerateCloudID() string
}

type cloudIDGenerator struct {
	prefix string
}

func NewCloudIDGenerator(prefix string) *cloudIDGenerator {
	if len(prefix) != 3 {
		panic(fmt.Errorf("cloud id prefix '%s' length != 3", prefix))
	}

	return &cloudIDGenerator{
		prefix: prefix,
	}
}

func (g *cloudIDGenerator) GenerateCloudID() string {
	randBytes := make([]byte, 11)
	if _, err := rand.Read(randBytes); err != nil {
		panic(fmt.Errorf("random generator error: %w", err))
	}

	enc := base32.StdEncoding.WithPadding(base32.NoPadding)

	// here we got 18 characters of 11 bytes of data, but we need only 17
	suffix := enc.EncodeToString(randBytes)[:17]
	return strings.ToLower(g.prefix + suffix)
}
