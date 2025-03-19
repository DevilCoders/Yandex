package locks

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

type ReleaseAction struct {
	actions.BaseAction
}

type ReleaseParams struct {
	InstanceID string
}

func (lllp *ReleaseParams) validate() error {
	return valid.Struct(lllp,
		valid.Value(&lllp.InstanceID, rule.Required),
	)
}

type ReleaseResult struct {
	Lock *license.Lock
}

func NewReleaseAction(env *env.Env) *ReleaseAction {
	return &ReleaseAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *ReleaseAction) checkParams(ctx context.Context, params ReleaseParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license lock release params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license lock release params passed validation")
	return nil
}

func (a *ReleaseAction) Do(ctx context.Context, params ReleaseParams) (*ReleaseResult, error) {
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
	if err != nil {
		return nil, err
	}

	timeNow := utils.GetTimeNow()

	ll.State = license.UnlockedLockState
	ll.EndTime = timeNow
	ll.UpdatedAt = timeNow

	err = a.Adapters().DB().UpsertLicenseLock(ctx, ll, tx)
	if err != nil {
		return nil, err
	}

	return &ReleaseResult{Lock: ll}, nil
}
