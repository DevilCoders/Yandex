package db

import (
	"context"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
)

type LicenseServerAdapter interface {
	UpsertOperation(ctx context.Context, op *model.Operation, tx *sqlx.Tx) error
	GetOperation(ctx context.Context, id string, tx *sqlx.Tx) (*model.Operation, error)

	GetLicenseTemplateByID(ctx context.Context, id string, tx *sqlx.Tx) (*license.Template, error)
	UpsertLicenseTemplate(ctx context.Context, lt *license.Template, tx *sqlx.Tx) error
	ListLicenseTemplatesByTariffID(ctx context.Context, publisherID, productID, tariffID string, tx *sqlx.Tx) ([]*license.Template, error)

	GetLicenseTemplateVersionByID(ctx context.Context, id string, tx *sqlx.Tx) (*license.TemplateVersion, error)
	UpsertLicenseTemplateVersion(ctx context.Context, ltv *license.TemplateVersion, tx *sqlx.Tx) error
	GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) (*license.TemplateVersion, error)
	ListLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) ([]*license.TemplateVersion, error)

	GetLicenseInstanceByID(ctx context.Context, id string, tx *sqlx.Tx) (*license.Instance, error)
	UpsertLicenseInstance(ctx context.Context, li *license.Instance, tx *sqlx.Tx) error
	GetLicenseInstancesByCloudID(ctx context.Context, cloudID string, tx *sqlx.Tx) ([]*license.Instance, error)
	GetRecreativeInstances(ctx context.Context, timeNow time.Time, tx *sqlx.Tx) ([]*license.Instance, error)
	GetPendingInstances(ctx context.Context, timeNow time.Time, tx *sqlx.Tx) ([]*license.Instance, error)

	UpsertLicenseLock(ctx context.Context, ll *license.Lock, tx *sqlx.Tx) error
	GetLicenseLockByInstanceID(ctx context.Context, licenseInstanceID string, tx *sqlx.Tx) (*license.Lock, error)
	UnlockLicenseLocksByLicenseInstanceID(ctx context.Context, billingAccountID string, endTime time.Time, tx *sqlx.Tx) error

	CreateTx(ctx context.Context) (*sqlx.Tx, error)
	CommitTx(ctx context.Context, errIn *error, tx *sqlx.Tx)
}
