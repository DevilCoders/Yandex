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

type ListAction struct {
	actions.BaseAction
}

type ListParams struct {
	CloudID string
}

func (lilp *ListParams) validate() error {
	return valid.Struct(lilp,
		valid.Value(&lilp.CloudID, rule.Required),
	)
}

type ListResult struct {
	Instances []*license.Instance
}

func NewListAction(env *env.Env) *ListAction {
	return &ListAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *ListAction) checkParams(ctx context.Context, params ListParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license instance list params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license instance list params passed validation")
	return nil
}

func (a *ListAction) Do(ctx context.Context, params ListParams) (*ListResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	lis, err := a.Adapters().DB().GetLicenseInstancesByCloudID(ctx, params.CloudID, tx)
	if err != nil {
		return nil, err
	}

	return &ListResult{
		Instances: lis,
	}, nil
}
