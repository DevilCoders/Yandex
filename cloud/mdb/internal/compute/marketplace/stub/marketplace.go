package mock

import (
	"context"

	marketplace "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func LicenseStub(allowCreateClusters bool) LicenseMock {
	return &licenseMock{
		AllowCreateClusters: allowCreateClusters,
	}
}

type LicenseMock interface {
	marketplace.LicenseService
	SetAllowCreateClusters(AllowCreateClusters bool)
}

type licenseMock struct {
	AllowCreateClusters bool
}

func (license *licenseMock) CheckLicenseResult(ctx context.Context, cloudID string, productIDs []string) error {
	if license.AllowCreateClusters {
		return nil
	}

	return semerr.Authorization("some license check error")
}

func (license *licenseMock) SetAllowCreateClusters(allow bool) {
	license.AllowCreateClusters = allow
}
