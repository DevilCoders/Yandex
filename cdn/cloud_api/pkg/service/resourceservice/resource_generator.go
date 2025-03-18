package resourceservice

import (
	"crypto/rand"
	"encoding/base32"
	"fmt"
	"strings"
)

// TODO: move to config
const (
	resourceIDPrefix = "cdn"
	cnameSuffix      = ".cdn.yandex.net"
)

// TODO: Rename?
type ResourceGenerator interface {
	ResourceID() (string, error)
	Cname(resourceID string) string
}

type ResourceGeneratorImpl struct {
	resourceIDPrefix string
	cnameSuffix      string
}

func NewResourceGenerator() *ResourceGeneratorImpl {
	return &ResourceGeneratorImpl{
		resourceIDPrefix: resourceIDPrefix,
		cnameSuffix:      cnameSuffix,
	}
}

func (g *ResourceGeneratorImpl) ResourceID() (string, error) {
	randBytes := make([]byte, 11)
	if _, err := rand.Read(randBytes); err != nil {
		return "", fmt.Errorf("random generator: %w", err)
	}

	enc := base32.StdEncoding.WithPadding(base32.NoPadding)

	// here we got 18 characters of 11 bytes of data, but we need only 17
	suffix := enc.EncodeToString(randBytes)[:17]

	return strings.ToLower(g.resourceIDPrefix + suffix), nil
}

func (g *ResourceGeneratorImpl) Cname(resourceID string) string {
	return resourceID + g.cnameSuffix
}
