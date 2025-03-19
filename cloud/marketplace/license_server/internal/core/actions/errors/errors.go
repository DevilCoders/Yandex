package errors

import (
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrLicenseTemplateIsNotActive            = xerrors.New("license template version is not active")
	ErrLicenseTemplateVersionIsAlreadyExists = xerrors.New("license template version is already exists")
	ErrLicenseInstanceAlreadyLocked          = xerrors.New("license instance already locked")
	ErrLicenseInstanceNotActive              = xerrors.New("license instance is not active")
	ErrLicenseTemplateNotPendingAndActive    = xerrors.New("license template is not pending and active")
	ErrLicenseTemplateVersionNotPending      = xerrors.New("license template version is not pending")
	ErrLicenseTemplateVersionNotDeprecated   = xerrors.New("license template version is not deprecated")
	ErrTariffNotActiveOrPending              = xerrors.New("tariff is not active or pending")
	ErrTariffNotPAYG                         = xerrors.New("tariff is not payg")
)
