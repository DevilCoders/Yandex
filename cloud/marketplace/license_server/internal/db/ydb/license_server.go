package ydb

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type LicenseServerProvider interface {
	UpsertOperation(ctx context.Context, op *Operation, tx *sqlx.Tx) error
	GetOperation(ctx context.Context, id string, tx *sqlx.Tx) (*Operation, error)

	GetLicenseTemplateByID(ctx context.Context, id string, tx *sqlx.Tx) (*LicenseTemplate, error)
	UpsertLicenseTemplate(ctx context.Context, lt *LicenseTemplate, tx *sqlx.Tx) error
	ListLicenseTemplatesByTariffID(ctx context.Context, publisherID, productID, tariffID string, tx *sqlx.Tx) ([]*LicenseTemplate, error)

	GetLicenseTemplateVersionByID(ctx context.Context, id string, tx *sqlx.Tx) (*LicenseTemplateVersion, error)
	UpsertLicenseTemplateVersion(ctx context.Context, lt *LicenseTemplateVersion, tx *sqlx.Tx) error
	GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) (*LicenseTemplateVersion, error)
	ListLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) ([]*LicenseTemplateVersion, error)

	GetLicenseInstanceByID(ctx context.Context, id string, tx *sqlx.Tx) (*LicenseInstance, error)
	UpsertLicenseInstance(ctx context.Context, li *LicenseInstance, tx *sqlx.Tx) error
	GetLicenseInstancesByCloudID(ctx context.Context, billingAccountID string, tx *sqlx.Tx) ([]*LicenseInstance, error)
	GetRecreativeInstances(ctx context.Context, timeNow ydb.UInt64Ts, tx *sqlx.Tx) ([]*LicenseInstance, error)
	GetPendingInstances(ctx context.Context, timeNow ydb.UInt64Ts, tx *sqlx.Tx) ([]*LicenseInstance, error)

	UpsertLicenseLock(ctx context.Context, ll *LicenseLock, tx *sqlx.Tx) error
	GetLicenseLockByInstanceID(ctx context.Context, instanceID string, tx *sqlx.Tx) (*LicenseLock, error)
	UnlockLicenseLocksByLicenseInstanceID(ctx context.Context, licenseInstanceID string, endTime ydb.UInt64Ts, tx *sqlx.Tx) error

	CreateTx(ctx context.Context) (*sqlx.Tx, error)
	Rollback(tx *sqlx.Tx) error
	Commit(tx *sqlx.Tx) error

	status.StatusChecker
}

type licenseServerProvider struct {
	*ydb.Connector
}

func NewLicenseServerProvider(c *ydb.Connector) LicenseServerProvider {
	return &licenseServerProvider{
		Connector: c,
	}
}

func (l *licenseServerProvider) CreateTx(ctx context.Context) (*sqlx.Tx, error) {
	tx, err := l.DB().BeginTxx(ctx, nil)
	if err != nil {
		return nil, err
	}

	return tx, nil
}

func (l *licenseServerProvider) Rollback(tx *sqlx.Tx) error {
	return tx.Rollback()
}

func (l *licenseServerProvider) Commit(tx *sqlx.Tx) error {
	return tx.Commit()
}
