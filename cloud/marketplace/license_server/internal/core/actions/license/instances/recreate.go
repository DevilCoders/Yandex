package instances

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

type RecreateAction struct {
	actions.BaseAction
}

type RecreateParams struct {
}

func (ligp *RecreateParams) validate() error {
	return nil
}

type RecreateResult struct {
	Instances []*license.Instance
}

func NewRecreateAction(env *env.Env) *RecreateAction {
	return &RecreateAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *RecreateAction) checkParams(ctx context.Context, params RecreateParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license instance recreate params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license instance recreate params passed validation")
	return nil
}

func (a *RecreateAction) Do(ctx context.Context, params RecreateParams) (*RecreateResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	timeNow := utils.GetTimeNow()
	instances, err := a.Adapters().DB().GetRecreativeInstances(ctx, timeNow, tx)
	if err != nil {
		return nil, err
	}
	if instances == nil {
		return &RecreateResult{}, nil
	}

	instanceIDs := make([]string, 0, len(instances))
	for i := range instances {
		instanceIDs = append(instanceIDs, instances[i].ID)
	}
	_ = instanceIDs
	// TODO: get their locks
	// TODO: unlock all locks
	// TODO: create new active instances from old ones
	// TODO: create locks for them
	// TODO: create operations for them

	return &RecreateResult{}, nil
}
