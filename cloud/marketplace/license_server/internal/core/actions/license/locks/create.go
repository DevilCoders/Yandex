package locks

import (
	"context"
	errors_is "errors"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/errors"
	ydb_errors "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type CreateAction struct {
	actions.BaseAction
}

type CreateParams struct {
	InstanceID     string
	ResourceLockID string
}

func (lllp *CreateParams) validate() error {
	return valid.Struct(lllp,
		valid.Value(&lllp.InstanceID, rule.Required),
		valid.Value(&lllp.ResourceLockID, rule.Required),
	)
}

type CreateResult struct {
	Lock *license.Lock
}

func NewCreateAction(env *env.Env) *CreateAction {
	return &CreateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *CreateAction) checkParams(ctx context.Context, params CreateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license lock create params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license lock create params passed validation")
	return nil
}

func (a *CreateAction) Do(ctx context.Context, params CreateParams) (*CreateResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	ll, err := a.Adapters().DB().GetLicenseLockByInstanceID(
		ctx,
		params.InstanceID,
		tx,
	)
	if errors_is.Is(err, ydb_errors.ErrNotFoundLicenseLock) {
		err = nil
	} else {
		if err != nil {
			return nil, err
		}

		if ll != nil {
			return nil, errors.ErrLicenseInstanceAlreadyLocked
		}
	}

	li, err := a.Adapters().DB().GetLicenseInstanceByID(ctx, params.InstanceID, tx)
	if err != nil {
		return nil, err
	}

	timeNow := utils.GetTimeNow()

	if li.State != license.ActiveInstanceState && li.State != license.CancelledInstanceState {
		err = errors.ErrLicenseInstanceNotActive
		return nil, err
	}

	id := a.CloudIDGenerator().GenerateCloudID()

	llNew := &license.Lock{
		ID:             id,
		InstanceID:     params.InstanceID,
		ResourceLockID: params.ResourceLockID,
		StartTime:      timeNow,
		EndTime:        li.EndTime,
		CreatedAt:      timeNow,
		UpdatedAt:      timeNow,
		State:          license.LockedLockState,
	}

	err = a.Adapters().DB().UpsertLicenseLock(ctx, llNew, tx)
	if err != nil {
		return nil, err
	}

	return &CreateResult{
		Lock: llNew,
	}, nil
}
