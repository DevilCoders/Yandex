package instances

import (
	"context"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

type ActivatePendingAction struct {
	actions.BaseAction
}

type ActivatePendingParams struct {
}

func (ligp *ActivatePendingParams) validate() error {
	return nil
}

type ActivatePendingResult struct {
	Instances []*license.Instance
}

func NewActivatePendingAction(env *env.Env) *ActivatePendingAction {
	return &ActivatePendingAction{
		BaseAction: actions.BaseAction{Env: env},
	}
}

func (a *ActivatePendingAction) checkParams(ctx context.Context, params ActivatePendingParams) error {
	err := params.validate()
	if err != nil {
		ctxtools.Logger(ctx).Debug("license instance activate pending params did not pass validation")
		return err
	}
	ctxtools.Logger(ctx).Debug("license instance activate pending params passed validation")
	return nil
}

func (a *ActivatePendingAction) Do(ctx context.Context, params ActivatePendingParams) (*ActivatePendingResult, error) {
	if err := a.checkParams(ctx, params); err != nil {
		return nil, err
	}

	tx, err := a.Adapters().DB().CreateTx(ctx)
	if err != nil {
		return nil, err
	}
	defer a.Adapters().DB().CommitTx(ctx, &err, tx)

	timeNow := utils.GetTimeNow()
	instances, err := a.Adapters().DB().GetPendingInstances(ctx, timeNow, tx)
	if err != nil {
		return nil, err
	}
	if instances == nil {
		return &ActivatePendingResult{}, nil
	}

	for i := range instances {
		instances[i].State = license.ActiveInstanceState
		diffTime := timeNow.Sub(instances[i].StartTime)
		instances[i].StartTime = timeNow
		instances[i].EndTime = instances[i].EndTime.Add(diffTime)
		instances[i].UpdatedAt = timeNow
	}
	// TODO: batch upsert
	// TODO: create operations for them

	return &ActivatePendingResult{Instances: instances}, nil
}
