package actions

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrNoCloudID       = xerrors.New("no cloud id provided")
	ErrEmptyProductIDs = xerrors.New("empty products ids list")

	ErrLicensedInstancePoolValue = xerrors.New("invalid lincensed instance pool")

	ErrRMPermissionDenied = xerrors.NewSentinel("resource service: permission denied")
	ErrCloudIDNotFound    = xerrors.NewSentinel("cloud id not found")

	ErrBackendTimeout = xerrors.NewSentinel("backend timeout")
)

type ErrLicenseCheck struct {
	CloudID     string
	ProductsIDs []string
}

func (e ErrLicenseCheck) Error() string {
	return fmt.Sprintf("Product license prohibits to use product(s) [%s] within cloud %s",
		strings.Join(e.ProductsIDs, ", "),
		e.CloudID,
	)
}

type ErrNotfoundCloudID struct {
	CloudID string
}

func (e ErrNotfoundCloudID) Error() string {
	return fmt.Sprintf("Cloud '%s' was not found", e.CloudID)
}

type ErrLicenseCheckExternal struct {
	ErrLicenseCheck
	ExternalMessages []string
}

func newErrLicenseCheckExternal(cloudID string, productsIDs []string, external []string) ErrLicenseCheckExternal {
	return ErrLicenseCheckExternal{
		ErrLicenseCheck: ErrLicenseCheck{
			CloudID:     cloudID,
			ProductsIDs: productsIDs,
		},

		ExternalMessages: external,
	}
}

func (e ErrLicenseCheckExternal) Error() string {
	var b strings.Builder

	_, _ = fmt.Fprintf(&b, "%s.", e.ErrLicenseCheck.Error())
	if len(e.ExternalMessages) > 0 {
		_, _ = fmt.Fprintf(&b, " Details %s.", strings.Join(e.ExternalMessages, ". "))
	}

	return b.String()
}
