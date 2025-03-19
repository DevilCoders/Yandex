package instances

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type DeleteAction struct {
	actions.BaseAction
}

type DeleteParams struct {
	ID string
}

func (ligp *DeleteParams) validate() error {
	return valid.Struct(ligp,
		valid.Value(&ligp.ID, rule.Required),
	)
}

func NewDeleteAction(env *env.Env) *DeleteAction {
	return &DeleteAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *DeleteAction) checkParams(ctx context.Context, params DeleteParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license instance delete params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license instance delete params passed validation")
	return nil
}

func (a *DeleteAction) Do(ctx context.Context, params DeleteParams) error {
	if err := a.checkParams(ctx, params); err != nil {
		return err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	li, err := a.Adapters().DB().GetLicenseInstanceByID(ctx, params.ID, tx)
	if err != nil {
		return err
	}

	timeNow := utils.GetTimeNow()

	if li.State == license.ActiveInstanceState || li.State == license.CancelledInstanceState {
		err = a.Adapters().DB().UnlockLicenseLocksByLicenseInstanceID(ctx, li.ID, timeNow, tx)
		if err != nil {
			return err
		}
	}
	li.State = license.DeletedInstanceState
	li.UpdatedAt = timeNow

	err = a.Adapters().DB().UpsertLicenseInstance(ctx, li, tx)
	if err != nil {
		return err
	}

	return nil
}
