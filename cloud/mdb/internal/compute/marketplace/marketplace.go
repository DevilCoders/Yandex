package marketplace

import (
	"context"
)

//go:generate ../../../scripts/mockgen.sh LicenseService

// LicenseService defines client to license service
type LicenseService interface {
	CheckLicenseResult(ctx context.Context, cloudID string, productIDs []string) error
}
