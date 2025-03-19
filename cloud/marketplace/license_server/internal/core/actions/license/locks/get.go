package locks

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/valid/v2"
	"a.yandex-team.ru/library/go/valid/v2/rule"
)

type GetAction struct {
	actions.BaseAction
}

type GetParams struct {
	InstanceID string
}

func (llgp *GetParams) validate() error {
	return valid.Struct(llgp,
		valid.Value(&llgp.InstanceID, rule.Required),
	)
}

type GetResult struct {
	Lock *license.Lock
}

func NewGetAction(env *env.Env) *GetAction {
	return &GetAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *GetAction) checkParams(ctx context.Context, params GetParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license lock get params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license lock get params passed validation")
	return nil
}

func (a *GetAction) Do(ctx context.Context, params GetParams) (*GetResult, error) {
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

	return &GetResult{Lock: ll}, nil
}
