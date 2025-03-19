package ydb

import "golang.org/x/xerrors"

var (
	ErrNotFoundLicenseTemplate        = xerrors.New("no license template")
	ErrNotFoundLicenseTemplateVersion = xerrors.New("no license template version")
	ErrNotFoundLicenseInstance        = xerrors.New("no license instance")
	ErrNotFoundLicenseLock            = xerrors.New("no license lock")
	ErrNotFoundOperation              = xerrors.New("no operation")
)
