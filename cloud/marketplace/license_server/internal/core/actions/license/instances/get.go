package instances

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
	ID string
}

func (ligp *GetParams) validate() error {
	return valid.Struct(ligp,
		valid.Value(&ligp.ID, rule.Required),
	)
}

type GetResult struct {
	Instance *license.Instance
}

func NewGetAction(env *env.Env) *GetAction {
	return &GetAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *GetAction) checkParams(ctx context.Context, params GetParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license instance get params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license instance get params passed validation")
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

	li, err := a.Adapters().DB().GetLicenseInstanceByID(ctx, params.ID, tx)
	if err != nil {
		return nil, err
	}

	return &GetResult{
		Instance: li,
	}, nil
}
